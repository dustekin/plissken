import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import song_to_sequence


class SongToSequenceDiagnosticsTests(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)
        self.song_path = self.root / "song.h"
        self.samples_path = self.root / "lm1_samples.h"
        self.song_path.write_text(
            """\
#define SONG_PPQ 96u
#define SONG_BPM 80u
static const song_event_t events[] = {
    {0u, 24u, 0, 60, 90},
    {1u, 24u, 0, 64, 70},
    {0u, 24u, 1, 48, 80},
    {0u, 24u, 2, 36, 90},
    {0u, 24u, 6, 60, 90},
    {12u, 24u, 6, 61, 80},
    {0u, 24u, 5, 0, 90},
    {0u, 24u, 3, 72, 90},
};
""",
            encoding="ascii",
        )
        self.samples_path.write_text(
            """\
static const int8_t rossflm1_kick[] = {1};
static const int8_t rossflm1_snare[] = {1};
static const int8_t rossflm1_hat_closed[] = {1};
""",
            encoding="ascii",
        )

    def tearDown(self):
        self.tempdir.cleanup()

    def test_analysis_counts_merges_voice_rejections_and_event_cap(self):
        with patch.object(song_to_sequence, "MAX_EVENTS", 2):
            _, _, events, stats = song_to_sequence.analyze(self.song_path, self.samples_path)

        self.assertEqual(stats["raw_event_count"], 8)
        self.assertEqual(stats["source_counts"][0], 2)
        self.assertEqual(stats["merged_counts"][0], 1)
        self.assertEqual(stats["merged_counts"][6], 1)
        self.assertEqual(stats["group_count"], 5)
        self.assertEqual(stats["voice_drop_counts"][6], 1)
        self.assertEqual(sum(stats["event_cap_drop_counts"].values()), 2)
        self.assertEqual(len(events), 2)
        self.assertEqual(stats["ignored_counts"][3], 1)

    def test_part_grid_override_only_refines_the_selected_part(self):
        baseline = song_to_sequence.analyze(self.song_path, self.samples_path)[3]
        refined = song_to_sequence.analyze(self.song_path, self.samples_path, {6: 1})[3]

        self.assertEqual(refined["pitch_grid_steps"][6], 1)
        self.assertEqual(refined["group_counts"][6], baseline["group_counts"][6] + 1)
        for part in (0, 1, 2, 5):
            self.assertEqual(refined["group_counts"][part], baseline["group_counts"][part])

    def test_part_quotas_preserve_requested_counts_and_sample_each_part(self):
        baseline_events = song_to_sequence.analyze(
            self.song_path, self.samples_path
        )[2]
        _, _, events, stats = song_to_sequence.analyze(
            self.song_path,
            self.samples_path,
            {6: 1},
            {0: 1, 1: 1, 6: 1},
            baseline_events,
        )

        self.assertEqual(len(events), 3)
        self.assertEqual(stats["kept_counts"], {0: 1, 1: 1, 6: 1})
        self.assertEqual(stats["part_quotas"], {0: 1, 1: 1, 6: 1})
        baseline_unchanged = {
            song_to_sequence.event_identity(event)
            for event in baseline_events
            if event["part"] in (0, 1)
        }
        selected_unchanged = {
            song_to_sequence.event_identity(event)
            for event in events
            if event["part"] in (0, 1)
        }
        self.assertEqual(selected_unchanged, baseline_unchanged)
        self.assertEqual(stats["baseline_preserved_counts"][0], 1)
        self.assertEqual(stats["baseline_preserved_counts"][1], 1)

    def test_report_mode_does_not_write_the_output_header(self):
        output_path = self.root / "escape_sequence.h"
        args = [
            "song_to_sequence.py",
            "--report",
            "--part-grid",
            "6=1",
            "--preserve-part-counts",
            str(self.song_path),
            str(self.samples_path),
            str(output_path),
        ]
        output = io.StringIO()
        with patch.object(sys, "argv", args), contextlib.redirect_stdout(output):
            song_to_sequence.main()

        self.assertFalse(output_path.exists())
        self.assertIn("Diagnostics only", output.getvalue())
        self.assertIn("preserves baseline per-part counts", output.getvalue())
        self.assertIn("Juno", output.getvalue())
        self.assertIn("dropped by event cap", output.getvalue())

    def test_grid_candidate_requires_and_writes_an_explicit_output(self):
        output_path = self.root / "escape_sequence_candidate.h"
        args = [
            "song_to_sequence.py",
            "--part-grid",
            "6=1",
            "--preserve-part-counts",
            str(self.song_path),
            str(self.samples_path),
            str(output_path),
        ]
        with patch.object(sys, "argv", args), contextlib.redirect_stdout(io.StringIO()):
            song_to_sequence.main()

        self.assertTrue(output_path.exists())
        self.assertIn(
            "#define ESCAPE_SEQUENCE_EVENT_COUNT 4u",
            output_path.read_text(encoding="ascii"),
        )


if __name__ == "__main__":
    unittest.main()
