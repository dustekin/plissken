"""Verify bass sample pitch, sustain/one-shot release, and the 808 level boost."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


RATE = 44100
SAMPLE_MODES = ((2, 3, 7), (4, 5, 8), (6, 7, 2), (8, 9, 1))
NOTE = 29


def render(host, directory, mode, sound):
    name = f"bass_mode_{mode}"
    scenario = directory / f"{name}.txt"
    delta = (mode - 2) % 10
    script = "wait 3200\nturn SELECT 3\nexpect instrument 4\n"
    if delta:
        script += f"turn PRESET {delta}\n"
    script += (
        f"expect bass_mode {mode}\n"
        f"expect bass_sound {sound}\n"
        f"wav {name}.wav\n"
        "key 0 down 100 0\n"
        "wait 100\n"
        "key 0 up\n"
        "wait 900\n"
        "wavstop\n"
    )
    scenario.write_text(script)
    result = subprocess.run(
        [str(host), str(scenario), str(directory)],
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"{name}: host render failed:\n{result.stdout}{result.stderr}")
    path = directory / f"{name}.wav"
    with wave.open(str(path), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError(f"{name}: expected 44.1 kHz, 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    stereo = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    return [(stereo[i] + stereo[i + 1]) * 0.5 for i in range(0, len(stereo), 2)]


def rms(samples):
    return math.sqrt(sum(sample * sample for sample in samples) / len(samples))


def tone_amplitude(samples, note):
    start, end = int(0.06 * RATE), int(0.18 * RATE)
    windowed = samples[start:end]
    frequency = 440.0 * 2.0 ** ((note - 69) / 12.0)
    coefficient = 2.0 * math.cos(2.0 * math.pi * frequency / RATE)
    previous = previous2 = 0.0
    window_sum = 0.0
    for i, sample in enumerate(windowed):
        window = 0.5 - 0.5 * math.cos(2.0 * math.pi * i / (len(windowed) - 1))
        value = sample * window
        current = value + coefficient * previous - previous2
        previous2, previous = previous, current
        window_sum += window
    power = previous * previous + previous2 * previous2 - coefficient * previous * previous2
    return 2.0 * math.sqrt(max(0.0, power)) / window_sum


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    levels = {}
    with tempfile.TemporaryDirectory(prefix="plissken-bass-samples-") as tmp:
        directory = Path(tmp)
        for sustain_mode, one_shot_mode, sound in SAMPLE_MODES:
            sustain = render(host, directory, sustain_mode, sound)
            one_shot = render(host, directory, one_shot_mode, sound)
            active = rms(sustain[int(0.06 * RATE) : int(0.18 * RATE)])
            sustain_tail = rms(sustain[int(0.30 * RATE) : int(0.40 * RATE)])
            one_shot_tail = rms(one_shot[int(0.30 * RATE) : int(0.40 * RATE)])
            fundamental = tone_amplitude(sustain, NOTE)
            peak = max(max(abs(x) for x in sustain), max(abs(x) for x in one_shot))
            tail_ratio = sustain_tail / active if active else 0.0
            levels[sustain_mode] = active
            print(
                f"bass mode {sustain_mode}: fundamental={fundamental:.1f}, "
                f"active RMS={active:.1f}, sustain tail ratio={tail_ratio:.3f}, "
                f"one-shot tail RMS={one_shot_tail:.1f}, peak={peak:.0f}"
            )
            if fundamental < 5.0 or active < 5.0 or peak > 32000:
                raise SystemExit(f"bass mode {sustain_mode}: invalid pitch, level, or peak")
            if tail_ratio > 0.05:
                raise SystemExit(f"bass mode {sustain_mode}: sustain did not respond to note-off")
            if one_shot_tail < active * 0.08:
                raise SystemExit(f"bass mode {one_shot_mode}: one-shot was cut off on note-off")

    if levels[6] < 2500.0:
        raise SystemExit("bass 808 did not reach the boosted output-level threshold")
    if levels[6] < levels[8] * 1.3:
        raise SystemExit("bass 808 did not remain noticeably louder than NoMore")
    print(f"808/NoMore active RMS ratio={levels[6] / levels[8]:.2f}")


if __name__ == "__main__":
    main()
