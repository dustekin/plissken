#!/usr/bin/env python3
"""Generate the built-in sequencer arrangement from the generated MIDI song header."""
import argparse
import math
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_PATH = ROOT / "dsp" / "escape_sequence.h"
MAX_EVENTS = 950
MAX_VOICES = 4
MAX_PITCH_DURATION_STEPS = 4
PITCH_GRID_STEPS = 2
DRUM_GRID_STEPS = 1
PITCH_PARTS = {0: (0, 0), 1: (1, 1), 2: (2, 2), 4: (2, 4), 6: (3, 6)}
DEFAULT_PARTS = (0, 1, 2, 5, 6)
PART_ORDER = (5, 0, 1, 2, 4, 6)
REPORT_PART_ORDER = (0, 1, 2, 4, 6, 5)
PART_NAMES = {
    0: "Juno",
    1: "Piano",
    2: "Pro-1",
    4: "Bass",
    5: "LM-1 drums",
    6: "CS-50",
}
DURATION_STEPS = (1, 2, 4, 8, 16, 32, 64)
DRUM_SAMPLE_NAMES = {0: "kick", 1: "snare", 2: "hat_closed"}
EVENT_PATTERN = re.compile(r"\{(\d+)u,\s*(\d+)u,\s*(\d+),\s*(\d+),\s*(\d+)\}")


def macro_value(text, name):
    match = re.search(rf"^#define {name} (\d+)u?$", text, re.MULTILINE)
    if not match:
        raise ValueError(f"Missing {name} in generated song header")
    return int(match.group(1))


def quantize(tick, ticks_per_step, grid_steps):
    return int(math.floor(tick / ticks_per_step / grid_steps + 0.5)) * grid_steps


def drum_lengths(samples_path):
    text = samples_path.read_text()
    lengths = {}
    for sample, name in DRUM_SAMPLE_NAMES.items():
        match = re.search(
            rf"static const int8_t rossflm1_{name}\[\] = \{{(.*?)\}};",
            text,
            re.DOTALL,
        )
        if not match:
            raise ValueError(f"Missing generated LM-1 sample {name} in {samples_path}")
        lengths[sample] = len(re.findall(r"-?\d+", match.group(1)))
    return lengths


def encode_event(event):
    step = event["step"]
    note = event["note"]
    duration = event["duration"]
    track = event["track"]
    instrument = event["instrument"]
    variant = event["variant"]
    return (
        step
        | (note << 12)
        | (duration << 19)
        | ((track & 3) << 22)
        | (instrument << 24)
        | (variant << 27)
        | (0x80000000 if track == 4 else 0)
    )


def event_identity(event):
    return tuple(
        event[field]
        for field in ("step", "note", "duration", "track", "instrument", "variant", "part")
    )


def analyze(
    song_path,
    samples_path,
    pitch_grid_overrides=None,
    part_quotas=None,
    baseline_events=None,
    included_parts=DEFAULT_PARTS,
    steps_per_bar=16,
    polyphonic_parts=(),
    priority_parts=None,
    part_transposes=None,
    rotate_priority=True,
    instrument_overrides=None,
):
    text = song_path.read_text()
    ppq = macro_value(text, "SONG_PPQ")
    bpm = macro_value(text, "SONG_BPM")
    ticks_per_step = ppq / 4
    if not ppq or ppq % 4:
        raise ValueError("MIDI PPQ must divide evenly into sequencer sixteenth steps")
    pitch_grid_overrides = pitch_grid_overrides or {}
    included_parts = tuple(included_parts)
    polyphonic_parts = tuple(polyphonic_parts)
    part_transposes = part_transposes or {}
    instrument_overrides = instrument_overrides or {}
    if not included_parts or any(part not in PITCH_PARTS and part != 5 for part in included_parts):
        raise ValueError("Invalid source-part selection")
    if any(part not in PITCH_PARTS or part not in included_parts for part in pitch_grid_overrides):
        raise ValueError("Pitch-grid overrides must use a supported pitched source part")
    if any(steps < 1 for steps in pitch_grid_overrides.values()):
        raise ValueError("Pitch-grid overrides must be at least one sixteenth step")
    if any(part not in included_parts or part == 5 for part in polyphonic_parts):
        raise ValueError("Polyphonic parts must be included pitched source parts")
    part_order = tuple(
        priority_parts or (part for part in PART_ORDER if part in included_parts)
    )
    if set(part_order) != set(included_parts):
        raise ValueError("Priority must list each included source part exactly once")
    if any(part not in included_parts or part == 5 for part in part_transposes):
        raise ValueError("Transpose values must use included pitched source parts")
    if any(
        part not in included_parts or part == 5 or not 0 <= instrument <= 7
        for part, instrument in instrument_overrides.items()
    ):
        raise ValueError("Instrument overrides must map included pitched parts to instruments 0-7")

    sample_lengths = drum_lengths(samples_path)
    groups = {}
    source_counts = Counter()
    ignored_counts = Counter()
    merged_counts = Counter()
    for tick, duration, part, note, velocity in (
        tuple(map(int, row)) for row in EVENT_PATTERN.findall(text)
    ):
        if part in PITCH_PARTS and part in included_parts:
            note += part_transposes.get(part, 0)
            if not 0 <= note <= 127:
                continue
            grid_steps = pitch_grid_overrides.get(part, PITCH_GRID_STEPS)
            step = quantize(tick, ticks_per_step, grid_steps)
            key = (part, note, step) if part in polyphonic_parts else (part, step)
        elif part == 5 and part in included_parts and note in DRUM_SAMPLE_NAMES:
            step = quantize(tick, ticks_per_step, DRUM_GRID_STEPS)
            key = (part, note, step)
        else:
            ignored_counts[part] += 1
            continue
        source_counts[part] += 1
        candidate = (tick, duration, part, note, velocity)
        if key in groups:
            merged_counts[part] += 1
        if key not in groups or (velocity, duration, note) > (
            groups[key][4],
            groups[key][1],
            groups[key][3],
        ):
            groups[key] = candidate

    candidates = []
    group_counts = Counter()
    onset_error_sum = Counter()
    onset_error_max = Counter()
    duration_error_sum = Counter()
    duration_error_max = Counter()
    duration_error_count = Counter()
    for key, (tick, duration, part, note, velocity) in groups.items():
        step = key[-1]
        group_counts[part] += 1
        onset_error = abs(step - tick / ticks_per_step)
        onset_error_sum[part] += onset_error
        onset_error_max[part] = max(onset_error_max[part], onset_error)
        if part == 5:
            duration_index = 7
            length = max(1, (sample_lengths[note] * 140 + 661499) // 661500)
            track, instrument, variant = 4, 5, note
        else:
            exact_duration_steps = duration / ticks_per_step
            rounded_steps = max(1, int(math.floor(exact_duration_steps + 0.5)))
            note_steps = min(
                DURATION_STEPS, key=lambda value: (abs(value - rounded_steps), value)
            )
            note_steps = min(note_steps, MAX_PITCH_DURATION_STEPS)
            duration_error = abs(note_steps - exact_duration_steps)
            duration_error_sum[part] += duration_error
            duration_error_max[part] = max(duration_error_max[part], duration_error)
            duration_error_count[part] += 1
            duration_index = DURATION_STEPS.index(note_steps)
            length = note_steps
            track, instrument = PITCH_PARTS[part]
            instrument = instrument_overrides.get(part, instrument)
            variant = 0
        candidates.append(
            {
                "step": step,
                "note": note,
                "duration": duration_index,
                "track": track,
                "instrument": instrument,
                "variant": variant,
                "part": part,
                "velocity": velocity,
                "end": step + length,
                "onset_error": onset_error,
            }
        )

    def priority(event):
        rotation = 0 if not rotate_priority else event["step"] // 2
        return (part_order.index(event["part"]) - rotation) % len(part_order)

    candidates.sort(key=lambda event: (event["step"], priority(event), -event["velocity"], event["note"]))
    accepted = []
    voice_drop_counts = Counter()
    for candidate in candidates:
        if any(
            sum(other["step"] <= step < other["end"] for other in accepted) >= MAX_VOICES
            for step in range(candidate["step"], candidate["end"])
        ):
            voice_drop_counts[candidate["part"]] += 1
            continue
        accepted.append(candidate)

    event_cap_drop_counts = Counter()
    if part_quotas is not None:
        if any(part not in part_order for part in part_quotas):
            raise ValueError("Part quotas must use supported source parts")
        if any(quota < 0 for quota in part_quotas.values()):
            raise ValueError("Part quotas cannot be negative")
        if sum(part_quotas.values()) > MAX_EVENTS:
            raise ValueError(f"Part quotas exceed the {MAX_EVENTS}-event limit")
        selected_indices = set()
        baseline_preserved_counts = Counter()
        if baseline_events is not None:
            candidate_indices = {}
            for index, event in enumerate(accepted):
                candidate_indices.setdefault(event_identity(event), []).append(index)
            for event in baseline_events:
                part = event["part"]
                if baseline_preserved_counts[part] >= part_quotas.get(part, 0):
                    continue
                for index in candidate_indices.get(event_identity(event), ()):
                    if index not in selected_indices:
                        selected_indices.add(index)
                        baseline_preserved_counts[part] += 1
                        break
        for part in part_order:
            indices = [
                index for index, event in enumerate(accepted) if event["part"] == part
            ]
            quota = part_quotas.get(part, 0)
            if quota > len(indices):
                raise ValueError(
                    f"{PART_NAMES[part]} quota requests {quota} events, "
                    f"but only {len(indices)} survive the voice limit"
                )
            remaining = quota - sum(
                accepted[index]["part"] == part for index in selected_indices
            )
            available_indices = [
                index for index in indices if index not in selected_indices
            ]
            for slot in range(remaining):
                local_index = (2 * slot + 1) * len(available_indices) // (
                    2 * remaining
                )
                selected_indices.add(available_indices[local_index])
        event_cap_drop_counts.update(
            event["part"]
            for index, event in enumerate(accepted)
            if index not in selected_indices
        )
        accepted = [
            event for index, event in enumerate(accepted) if index in selected_indices
        ]
    elif len(accepted) > MAX_EVENTS:
        retained_indices = {
            index * len(accepted) // MAX_EVENTS for index in range(MAX_EVENTS)
        }
        event_cap_drop_counts.update(
            event["part"]
            for index, event in enumerate(accepted)
            if index not in retained_indices
        )
        accepted = [
            event for index, event in enumerate(accepted) if index in retained_indices
        ]
    if not accepted:
        raise ValueError("No compatible sequencer events found")

    end_step = max(event["end"] for event in accepted)
    bars = (end_step + steps_per_bar - 1) // steps_per_bar
    if bars > 128:
        raise ValueError(f"Converted song exceeds the 128-bar sequencer limit: {bars} bars")
    if max(event["step"] for event in accepted) > 0xFFF:
        raise ValueError("Converted song exceeds the sequencer step encoding")
    kept_counts = Counter(event["part"] for event in accepted)
    kept_onset_error_sum = Counter()
    kept_onset_error_max = Counter()
    for event in accepted:
        part = event["part"]
        kept_onset_error_sum[part] += event["onset_error"]
        kept_onset_error_max[part] = max(
            kept_onset_error_max[part], event["onset_error"]
        )
    stats = {
        "raw_event_count": sum(source_counts.values()) + sum(ignored_counts.values()),
        "source_counts": source_counts,
        "ignored_counts": ignored_counts,
        "group_counts": group_counts,
        "merged_counts": merged_counts,
        "voice_drop_counts": voice_drop_counts,
        "event_cap_drop_counts": event_cap_drop_counts,
        "kept_counts": kept_counts,
        "onset_error_sum": onset_error_sum,
        "onset_error_max": onset_error_max,
        "group_count": sum(group_counts.values()),
        "duration_error_sum": duration_error_sum,
        "duration_error_max": duration_error_max,
        "duration_error_count": duration_error_count,
        "kept_onset_error_sum": kept_onset_error_sum,
        "kept_onset_error_max": kept_onset_error_max,
        "part_quotas": part_quotas,
        "baseline_preserved_counts": baseline_preserved_counts
        if part_quotas is not None
        else None,
        "pitch_grid_steps": {
            part: pitch_grid_overrides.get(part, PITCH_GRID_STEPS) for part in PITCH_PARTS
        },
        "included_parts": included_parts,
        "steps_per_bar": steps_per_bar,
        "polyphonic_parts": polyphonic_parts,
        "priority_parts": part_order,
        "part_transposes": part_transposes,
        "instrument_overrides": instrument_overrides,
    }
    return bpm, bars, accepted, stats


def convert(song_path, samples_path):
    bpm, bars, events, _ = analyze(song_path, samples_path)
    return bpm, bars, events


def print_diagnostics(song_path, bpm, bars, stats):
    print(f"Diagnostics only for {song_path}; no sequence header written.")
    print(
        f"Result: {sum(stats['kept_counts'].values())} events, {bars} bars at {bpm} BPM."
    )
    print(
        f"Limits: {MAX_VOICES} simultaneous voices, {MAX_EVENTS} events; "
        f"{stats['steps_per_bar']}/16 meter, pitched grid default {PITCH_GRID_STEPS} sixteenth steps, "
        f"drums every {DRUM_GRID_STEPS}, pitched note length cap "
        f"{MAX_PITCH_DURATION_STEPS} steps."
    )
    print(
        "Part          grid source groups merged voice-drop cap-drop kept "
        "onset-error avg/max duration-error avg/max"
    )
    print("              (sixteenth-step errors)")
    for part in REPORT_PART_ORDER:
        if part not in stats["included_parts"]:
            continue
        onset_count = stats["group_counts"][part]
        duration_count = stats["duration_error_count"][part]
        grid_steps = (
            DRUM_GRID_STEPS if part == 5 else stats["pitch_grid_steps"][part]
        )
        onset_errors = (
            f"{stats['onset_error_sum'][part] / onset_count:.2f}/"
            f"{stats['onset_error_max'][part]:.2f}"
            if onset_count
            else "-"
        )
        duration_errors = (
            f"{stats['duration_error_sum'][part] / duration_count:.2f}/"
            f"{stats['duration_error_max'][part]:.2f}"
            if duration_count
            else "-"
        )
        print(
            f"{PART_NAMES[part]:12} "
            f"{grid_steps:>4} "
            f"{stats['source_counts'][part]:>6} "
            f"{stats['group_counts'][part]:>6} "
            f"{stats['merged_counts'][part]:>6} "
            f"{stats['voice_drop_counts'][part]:>10} "
            f"{stats['event_cap_drop_counts'][part]:>9} "
            f"{stats['kept_counts'][part]:>4} "
            f"{onset_errors:>17} "
            f"{duration_errors:>22}"
        )
    ignored = ", ".join(
        f"{PART_NAMES.get(part, f'part {part}')}={count}"
        for part, count in sorted(stats["ignored_counts"].items())
    ) or "none"
    print(f"Ignored source events: {ignored}")
    print(
        f"Totals: {stats['raw_event_count']} source events, "
        f"{sum(stats['source_counts'].values())} compatible, "
        f"{stats['group_count']} grouped, "
        f"{sum(stats['voice_drop_counts'].values())} rejected by voice limit, "
        f"{sum(stats['event_cap_drop_counts'].values())} dropped by event cap, "
        f"{sum(stats['kept_counts'].values())} kept."
    )
    if stats["part_quotas"] is not None:
        print("Event allocation preserves baseline per-part counts.")
        preserved = ", ".join(
            f"{PART_NAMES[part]} {stats['baseline_preserved_counts'][part]}"
            for part in REPORT_PART_ORDER
            if stats["part_quotas"].get(part, 0)
        )
        print(f"Matching baseline events retained: {preserved}")
    kept_onset_errors = ", ".join(
        f"{PART_NAMES[part]} "
        f"{stats['kept_onset_error_sum'][part] / stats['kept_counts'][part]:.2f}/"
        f"{stats['kept_onset_error_max'][part]:.2f}"
        for part in REPORT_PART_ORDER
        if part in stats["included_parts"]
        if stats["kept_counts"][part]
    )
    print(f"Selected onset error avg/max (sixteenth steps): {kept_onset_errors}")
    print("MIDI velocity is used for candidate selection but is not stored in sequence events.")


def parse_part_grid(value):
    try:
        part_text, steps_text = value.split("=", 1)
        part, steps = int(part_text), int(steps_text)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("Expected PART=STEPS, for example 6=1") from exc
    if part not in PITCH_PARTS:
        raise argparse.ArgumentTypeError(f"Unsupported pitched source part: {part}")
    if steps < 1:
        raise argparse.ArgumentTypeError("Grid steps must be at least 1")
    return part, steps


def parse_parts(value):
    try:
        parts = tuple(int(part.strip()) for part in value.split(","))
    except ValueError as exc:
        raise argparse.ArgumentTypeError("Expected comma-separated part numbers") from exc
    if not parts or any(part not in PITCH_PARTS and part != 5 for part in parts):
        raise argparse.ArgumentTypeError("Unsupported source part")
    if len(set(parts)) != len(parts):
        raise argparse.ArgumentTypeError("Source parts must be unique")
    return parts


def parse_quotas(values):
    quotas = {}
    for value in values:
        try:
            part_text, count_text = value.split("=", 1)
            part, count = int(part_text), int(count_text)
        except ValueError as exc:
            raise argparse.ArgumentTypeError("Expected PART=COUNT, for example 0=15") from exc
        if part in quotas or count < 0:
            raise argparse.ArgumentTypeError("Quota parts must be unique and counts nonnegative")
        quotas[part] = count
    return quotas


def parse_transposes(values):
    transposes = {}
    for value in values:
        try:
            part_text, semitones_text = value.split("=", 1)
            part, semitones = int(part_text), int(semitones_text)
        except ValueError as exc:
            raise argparse.ArgumentTypeError(
                "Expected PART=SEMITONES, for example 1=-12"
            ) from exc
        if part in transposes:
            raise argparse.ArgumentTypeError("Transpose parts must be unique")
        transposes[part] = semitones
    return transposes


def parse_instruments(values):
    instruments = {}
    for value in values:
        try:
            part_text, instrument_text = value.split("=", 1)
            part, instrument = int(part_text), int(instrument_text)
        except ValueError as exc:
            raise argparse.ArgumentTypeError(
                "Expected PART=INSTRUMENT, for example 1=6"
            ) from exc
        if part in instruments or not 0 <= instrument <= 7:
            raise argparse.ArgumentTypeError(
                "Instrument parts must be unique and instruments must be 0-7"
            )
        instruments[part] = instrument
    return instruments


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--instrument",
        action="append",
        default=[],
        metavar="PART=INSTRUMENT",
        help="replace a source part's renderer instrument ID; repeat per part",
    )
    parser.add_argument(
        "--strict-priority",
        action="store_true",
        help="do not rotate source-part priority when resolving voice collisions",
    )
    parser.add_argument(
        "--transpose",
        action="append",
        default=[],
        metavar="PART=SEMITONES",
        help="transpose a retained pitched part; repeat per part",
    )
    parser.add_argument(
        "--report",
        action="store_true",
        help="report conversion losses without writing the generated sequence header",
    )
    parser.add_argument(
        "--parts",
        type=parse_parts,
        default=DEFAULT_PARTS,
        help="comma-separated source parts to retain (default: 0,1,2,5,6)",
    )
    parser.add_argument(
        "--meter",
        choices=("4/4", "5/4", "8/4", "8/8", "16/16"),
        default="4/4",
        help="sequencer meter for generated bar boundaries",
    )
    parser.add_argument(
        "--symbol-prefix",
        default="ESCAPE",
        help="generated macro and event-array prefix",
    )
    parser.add_argument(
        "--polyphonic-parts",
        type=parse_parts,
        default=(),
        help="comma-separated source parts whose simultaneous pitches must be retained",
    )
    parser.add_argument(
        "--priority",
        type=parse_parts,
        help="part order used to resolve simultaneous-voice collisions",
    )
    parser.add_argument(
        "--quota",
        action="append",
        default=[],
        metavar="PART=COUNT",
        help="exact retained event quota for a source part; repeat per part",
    )
    parser.add_argument(
        "--part-grid",
        action="append",
        type=parse_part_grid,
        metavar="PART=STEPS",
        help="override a pitched source part's onset grid for a report or candidate",
    )
    parser.add_argument(
        "--preserve-part-counts",
        action="store_true",
        help="retain baseline per-part event counts for a grid override",
    )
    parser.add_argument("song_path", nargs="?", type=Path, default=ROOT / "dsp" / "song.h")
    parser.add_argument(
        "samples_path", nargs="?", type=Path, default=ROOT / "build" / "gen" / "lm1_samples.h"
    )
    parser.add_argument("output_path", nargs="?", type=Path)
    args = parser.parse_args()
    if args.part_grid and not args.report and not args.preserve_part_counts:
        parser.error("--part-grid generation requires --preserve-part-counts")
    if args.preserve_part_counts and not args.part_grid:
        parser.error("--preserve-part-counts requires --part-grid")
    if args.part_grid and not args.report and args.output_path is None:
        parser.error("part-grid generation requires an explicit output path")
    if not args.symbol_prefix.replace("_", "").isalnum():
        parser.error("--symbol-prefix must contain only letters, digits and underscores")
    steps_per_bar = {"4/4": 16, "5/4": 20, "8/4": 32, "8/8": 16, "16/16": 16}[args.meter]
    meter_index = {"4/4": 0, "8/4": 1, "8/8": 2, "16/16": 3, "5/4": 4}[args.meter]
    if set(args.polyphonic_parts) - set(args.parts):
        parser.error("--polyphonic-parts must be included by --parts")
    priority_parts = args.priority or tuple(part for part in PART_ORDER if part in args.parts)
    if set(priority_parts) != set(args.parts):
        parser.error("--priority must list each --parts value exactly once")
    quotas = parse_quotas(args.quota)
    transposes = parse_transposes(args.transpose)
    instruments = parse_instruments(args.instrument)
    if quotas and set(quotas) != set(args.parts):
        parser.error("--quota must specify every --parts value")
    if set(transposes) - set(args.parts):
        parser.error("--transpose must use parts included by --parts")
    if set(instruments) - set(args.parts) or 5 in instruments:
        parser.error("--instrument must use pitched parts included by --parts")
    part_quotas = None
    baseline_events = None
    if args.preserve_part_counts:
        _, _, baseline_events, baseline_stats = analyze(
            args.song_path,
            args.samples_path,
            included_parts=args.parts,
            steps_per_bar=steps_per_bar,
            polyphonic_parts=args.polyphonic_parts,
            priority_parts=priority_parts,
            part_transposes=transposes,
            rotate_priority=not args.strict_priority,
            instrument_overrides=instruments,
        )
        part_quotas = baseline_stats["kept_counts"]
    elif quotas:
        part_quotas = quotas
    bpm, bars, events, stats = analyze(
        args.song_path,
        args.samples_path,
        dict(args.part_grid or ()),
        part_quotas,
        baseline_events,
        args.parts,
        steps_per_bar,
        args.polyphonic_parts,
        priority_parts,
        transposes,
        not args.strict_priority,
        instruments,
    )
    if args.report:
        print_diagnostics(args.song_path, bpm, bars, stats)
        return
    output_path = args.output_path or DEFAULT_OUTPUT_PATH
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="\n") as output:
        output.write("/* Generated by tools/song_to_sequence.py; do not edit. */\n")
        prefix = args.symbol_prefix.upper()
        output.write(f"#define {prefix}_SEQUENCE_BPM {bpm}u\n")
        output.write(f"#define {prefix}_SEQUENCE_BARS {bars}u\n")
        output.write(f"#define {prefix}_SEQUENCE_METER {meter_index}u\n")
        output.write(f"#define {prefix}_SEQUENCE_EVENT_COUNT {len(events)}u\n")
        output.write(
            f"static const uint32_t {prefix.lower()}_sequence_events"
            f"[{prefix}_SEQUENCE_EVENT_COUNT] = {{\n"
        )
        for offset in range(0, len(events), 4):
            packed = ", ".join(f"0x{encode_event(event):08X}u" for event in events[offset : offset + 4])
            output.write(f"    {packed},\n")
        output.write("};\n")
    counts = {part: sum(event["part"] == part for event in events) for part in PART_ORDER}
    print(f"{len(events)} events, {bars} bars at {bpm} BPM, source parts {counts} -> {output_path}")


if __name__ == "__main__":
    main()
