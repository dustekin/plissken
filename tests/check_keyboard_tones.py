"""Verify Juno, piano and Pro One keyboard pitch and velocity response."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


RATE = 44100
INSTRUMENTS = (0, 1, 2)
KEYS = (0, 13, 26)
JUNO_CHORD_KEYS = tuple(range(19, 27))


def render(host, directory, instrument, key, velocity):
    select_delta = instrument - 1
    name = f"voice_{instrument}_{key}_{velocity}"
    scenario = directory / f"{name}.txt"
    scenario.write_text(
        "wait 2000\n"
        f"turn SELECT {select_delta}\n"
        f"expect instrument {instrument}\n"
        f"wav {name}.wav\n"
        f"key {key} down {velocity} 0\n"
        "wait 400\n"
        f"key {key} up\n"
        "wait 100\n"
        "wavstop\n"
    )
    subprocess.run([str(host), str(scenario), str(directory)], check=True, stdout=subprocess.DEVNULL)
    with wave.open(str(directory / f"{name}.wav"), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError(f"{name}: expected 44.1 kHz, 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    stereo = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    return [(stereo[i] + stereo[i + 1]) * 0.5 for i in range(0, len(stereo), 2)]


def tone_amplitude(samples, note):
    start, end = int(0.15 * RATE), int(0.30 * RATE)
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


def rms(samples):
    return math.sqrt(sum(sample * sample for sample in samples) / len(samples))


def render_juno_chord(host, directory):
    scenario = directory / "juno_eight_voice.txt"
    script = "wait 2000\nturn SELECT -1\nexpect instrument 0\nwav juno_eight_voice.wav\n"
    for key in JUNO_CHORD_KEYS:
        script += f"key {key} down 100 0\n"
    script += f"expect keys {sum(1 << key for key in JUNO_CHORD_KEYS)}\nwait 400\n"
    for key in JUNO_CHORD_KEYS:
        script += f"key {key} up\n"
    script += "wait 100\nwavstop\n"
    scenario.write_text(script)
    result = subprocess.run(
        [str(host), str(scenario), str(directory)],
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"Juno eight-voice host render failed:\n{result.stdout}{result.stderr}")
    with wave.open(str(directory / "juno_eight_voice.wav"), "rb") as wav:
        if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
            raise ValueError("Juno chord: expected 44.1 kHz, 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    stereo = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    return [(stereo[i] + stereo[i + 1]) * 0.5 for i in range(0, len(stereo), 2)]


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-keyboard-tones-") as tmp:
        directory = Path(tmp)
        renders = {}
        for instrument in INSTRUMENTS:
            for key in KEYS:
                audio = render(host, directory, instrument, key, 100)
                expected_note = 53 + key
                level = tone_amplitude(audio, expected_note)
                measured_rms = rms(audio)
                peak = max(abs(sample) for sample in audio)
                fundamental_ratio = level / measured_rms if measured_rms else 0.0
                print(
                    f"instrument {instrument} note {expected_note}: "
                    f"fundamental={level:.1f} ({fundamental_ratio:.2f}x RMS), "
                    f"RMS={measured_rms:.1f}, peak={peak:.0f}"
                )
                if level < 10.0 or fundamental_ratio < 0.1 or measured_rms < 5.0 or peak > 32000:
                    raise SystemExit(f"instrument {instrument} note {expected_note}: invalid render")
                renders[instrument, key, 100] = audio

        for instrument in INSTRUMENTS:
            soft = render(host, directory, instrument, 13, 20)
            loud = renders[instrument, 13, 100]
            ratio = rms(loud) / rms(soft) if rms(soft) else 0.0
            print(f"instrument {instrument} velocity 20→100 RMS ratio={ratio:.2f}")
            if ratio < 1.5:
                raise SystemExit(f"instrument {instrument}: velocity response was not measurable")

        chord = render_juno_chord(host, directory)
        chord_levels = [tone_amplitude(chord, 53 + key) for key in JUNO_CHORD_KEYS]
        peak = max(chord_levels)
        ratios = [level / peak if peak else 0.0 for level in chord_levels]
        print("Juno eight-voice fundamentals: " + ", ".join(
            f"{53 + key}={level:.1f} ({ratio:.2f}x)" for key, level, ratio in zip(JUNO_CHORD_KEYS, chord_levels, ratios)
        ))
        if peak < 10.0 or min(ratios) < 0.05 or max(abs(sample) for sample in chord) > 32000:
            raise SystemExit("Juno eight-note chord lost a voice or exceeded the output headroom limit")


if __name__ == "__main__":
    main()
