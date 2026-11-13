"""Verify that the Pro One chord renderer sustains all three chord tones."""
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import wave


RATE = 44100
NOTES = (56, 60, 63)
SUB_NOTES = tuple(note - 12 for note in NOTES)


def render(host, directory):
    scenario = directory / "pro1_chord.txt"
    scenario.write_text(
        "wait 2000\n"
        "turn SELECT 6\n"
        "expect instrument 7\n"
        "press EDIT\n"
        "turn SELECT 3\n"
        "release EDIT\n"
        "expect chord_voice 3\n"
        "turn K1 -5\n"
        "turn K2 -5\n"
        "turn K3 -5\n"
        "turn K4 -5\n"
        "wav pro1_chord.wav\n"
        "key 3 down 100\n"
        "wait 400\n"
        "key 3 up\n"
        "wait 100\n"
        "wavstop\n"
    )
    subprocess.run([str(host), str(scenario), str(directory)], check=True)
    return directory / "pro1_chord.wav"


def read_window(path):
    with wave.open(str(path), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError(f"{path}: expected 44.1 kHz, 16-bit stereo audio")
        start = int(0.1 * RATE)
        end = int(0.4 * RATE)
        wav.setpos(start)
        raw = wav.readframes(end - start)
    count = len(raw) // 4
    left = [int.from_bytes(raw[i * 4:i * 4 + 2], "little", signed=True) for i in range(count)]
    right = [int.from_bytes(raw[i * 4 + 2:i * 4 + 4], "little", signed=True) for i in range(count)]
    return [(left[i] + right[i]) * 0.5 for i in range(count)]


def tone_amplitude(samples, note):
    frequency = 440.0 * 2.0 ** ((note - 69) / 12.0)
    coefficient = 2.0 * math.cos(2.0 * math.pi * frequency / RATE)
    previous = previous2 = 0.0
    window_sum = 0.0
    size = len(samples)
    for i, sample in enumerate(samples):
        window = 0.5 - 0.5 * math.cos(2.0 * math.pi * i / (size - 1))
        value = sample * window
        current = value + coefficient * previous - previous2
        previous2, previous = previous, current
        window_sum += window
    power = previous * previous + previous2 * previous2 - coefficient * previous * previous2
    return 2.0 * math.sqrt(max(0.0, power)) / window_sum


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-chord-render-") as tmp:
        directory = Path(tmp)
        audio = read_window(render(host, directory))
    levels = [tone_amplitude(audio, note) for note in NOTES]
    sub_levels = [tone_amplitude(audio, note) for note in SUB_NOTES]
    peak = max(levels)
    ratios = [level / peak if peak else 0.0 for level in levels]
    print("Pro One chord note amplitudes: " + ", ".join(f"{n}={v:.4f} ({r:.2f}x)" for n, v, r in zip(NOTES, levels, ratios)))
    if peak < 100.0 or min(ratios) < 0.08:
        raise SystemExit("Pro One chord does not sustain all three chord tones")
    print("Pro One octave-sub amplitudes: " + ", ".join(f"{n}={v:.4f}" for n, v in zip(SUB_NOTES, sub_levels)))
    if min(sub_levels) < 10.0:
        raise SystemExit("Pro One chord is missing one or more octave-sub tones")


if __name__ == "__main__":
    main()
