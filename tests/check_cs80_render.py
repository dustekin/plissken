"""Exercise the independent CS-80-inspired patches and continuous controls."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


RATE = 44100
PATCH_CONTROLS = (
    (72, 28, 62, 92),
    (88, 14, 54, 88),
    (62, 34, 72, 96),
    (56, 42, 64, 94),
    (74, 20, 58, 90),
    (66, 38, 58, 92),
    (54, 24, 46, 96),
    (68, 18, 64, 90),
    (82, 52, 72, 94),
    (78, 32, 62, 92),
)


def tone_amplitude(samples, note):
    start, end = int(0.05 * RATE), int(0.20 * RATE)
    windowed = samples[start:end]
    frequency = 440.0 * 2.0 ** ((note - 69) / 12.0)
    coefficient = 2.0 * math.cos(2.0 * math.pi * frequency / RATE)
    previous = previous2 = 0.0
    window_sum = 0.0
    size = len(windowed)
    for i, sample in enumerate(windowed):
        window = 0.5 - 0.5 * math.cos(2.0 * math.pi * i / (size - 1))
        value = sample * window
        current = value + coefficient * previous - previous2
        previous2, previous = previous, current
        window_sum += window
    power = previous * previous + previous2 * previous2 - coefficient * previous * previous2
    return 2.0 * math.sqrt(max(0.0, power)) / window_sum


def render(host, directory, patch, control=None):
    scenario = directory / "cs80_render.txt"
    script = (
        "wait 2000\n"
        "turn SELECT 5\n"
        "expect instrument 6\n"
        "press EDIT\n"
        f"turn SELECT {patch}\n"
        "release EDIT\n"
        f"expect cs80_preset {patch}\n"
    )
    for knob, value in enumerate(PATCH_CONTROLS[patch - 1], 1):
        script += f"expect fx{knob} {value}\n"
    if control:
        knob, delta, expected = control
        script += f"turn K{knob} {delta}\nexpect fx{knob} {expected}\n"
    script += (
        f"wav cs80_{patch}_{control[0] if control else 0}.wav\n"
        "key 7 down 250\n"
        "wait 250\n"
        "key 7 up\n"
        "wait 450\n"
        "wavstop\n"
    )
    scenario.write_text(script)
    result = subprocess.run(
        [str(host), str(scenario), str(directory)],
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"CS-80 patch {patch} host render failed:\n{result.stdout}{result.stderr}")
    path = directory / f"cs80_{patch}_{control[0] if control else 0}.wav"
    with wave.open(str(path), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError(f"{path}: expected 44.1 kHz, 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    return struct.unpack("<" + "h" * (len(raw) // 2), raw)[::2]


def render_chord(host, directory):
    scenario = directory / "cs80_chord.txt"
    scenario.write_text(
        "wait 2000\n"
        "turn SELECT 5\n"
        "press EDIT\n"
        "turn SELECT 1\n"
        "release EDIT\n"
        "wav cs80_chord.wav\n"
        "key 2 down 100 0\n"
        "key 5 down 100 0\n"
        "key 7 down 100 0\n"
        "wait 250\n"
        "expect keys 164\n"
        "key 2 up\n"
        "key 5 up\n"
        "key 7 up\n"
        "wait 400\n"
        "wavstop\n"
    )
    result = subprocess.run(
        [str(host), str(scenario), str(directory)],
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"CS-80 chord host render failed:\n{result.stdout}{result.stderr}")
    path = directory / "cs80_chord.wav"
    with wave.open(str(path), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError(f"{path}: expected 44.1 kHz, 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    return struct.unpack("<" + "h" * (len(raw) // 2), raw)[::2]


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-cs80-render-") as tmp:
        directory = Path(tmp)
        renders = []
        for patch in range(1, len(PATCH_CONTROLS) + 1):
            audio = render(host, directory, patch)
            peak = max(abs(sample) for sample in audio)
            rms = math.sqrt(sum(sample * sample for sample in audio) / len(audio))
            print(f"CS-80 patch {patch}: RMS={rms:.1f}, peak={peak}")
            if rms < 5.0 or peak > 32700:
                raise SystemExit(f"CS-80 patch {patch} is silent or clips")
            if patch == 1:
                root_amplitude = tone_amplitude(audio, 48)
                print(f"CS-80 patch 1 octave-shifted fundamental: {root_amplitude:.1f}")
                if root_amplitude < 10.0:
                    raise SystemExit("CS-80 patch 1 does not render its expected octave-shifted pitch")
            renders.append(audio)

        chord = render_chord(host, directory)
        chord_levels = [tone_amplitude(chord, note) for note in (43, 46, 48)]
        print("CS-80 triad fundamentals: " + ", ".join(f"{note}={level:.1f}" for note, level in zip((43, 46, 48), chord_levels)))
        if min(chord_levels) < 10.0:
            raise SystemExit("CS-80 triad does not sustain all three expected notes")

        baseline = renders[0]
        for knob in range(1, 5):
            changed = render(host, directory, 1, (knob, 4, PATCH_CONTROLS[0][knob - 1] + 16))
            if abs(len(changed) - len(baseline)) > RATE // 25:
                raise SystemExit(f"CS-80 K{knob}: render duration changed unexpectedly")
            count = min(len(changed), len(baseline))
            energy = sum(sample * sample for sample in baseline[:count])
            difference = math.sqrt(sum((a - b) ** 2 for a, b in zip(baseline, changed)) / energy)
            print(f"CS-80 K{knob}: relative render difference={difference:.4f}")
            if difference < 0.005:
                raise SystemExit(f"CS-80 K{knob} has no measurable effect")


if __name__ == "__main__":
    main()
