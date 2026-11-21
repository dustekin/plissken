"""Verify sampled-piano pitch, velocity response, release, and output level."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


RATE = 44100


def render(host, directory, name, key, velocity, sampled):
    scenario = directory / f"{name}.txt"
    script = "wait 3200\nexpect instrument 1\n"
    if not sampled:
        script += "turn ALGO 1\n"
    script += (
        f"wav {name}.wav\n"
        f"key {key} down {velocity} 0\n"
        "wait 500\n"
        f"key {key} up\n"
        "wait 100\n"
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
    start, end = int(0.15 * RATE), int(0.35 * RATE)
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


def render_chord(host, directory):
    keys = (0, 5, 9, 13, 17, 26)
    scenario = directory / "piano_sample_chord.txt"
    script = "wait 3200\nexpect instrument 1\nwav piano_sample_chord.wav\n"
    for key in keys:
        script += f"key {key} down 90 0\n"
    script += "wait 300\n"
    for key in keys:
        script += f"key {key} up\n"
    script += "wait 100\nwavstop\n"
    scenario.write_text(script)
    result = subprocess.run(
        [str(host), str(scenario), str(directory)],
        text=True,
        capture_output=True,
    )
    if result.returncode:
        raise RuntimeError(f"sample piano chord render failed:\n{result.stdout}{result.stderr}")
    with wave.open(str(directory / "piano_sample_chord.wav"), "rb") as wav:
        raw = wav.readframes(wav.getnframes())
    stereo = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    return [(stereo[i] + stereo[i + 1]) * 0.5 for i in range(0, len(stereo), 2)]


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-piano-samples-") as tmp:
        directory = Path(tmp)
        synth = render(host, directory, "piano_synth", 13, 100, False)
        sample_renders = {}
        for key in (0, 13, 26):
            audio = render(host, directory, f"piano_sample_{key}", key, 100, True)
            note = 53 + key
            fundamental = tone_amplitude(audio, note)
            level = rms(audio[int(0.15 * RATE) : int(0.35 * RATE)])
            peak = max(abs(sample) for sample in audio)
            ratio = fundamental / level if level else 0.0
            print(f"sample piano note {note}: fundamental={fundamental:.1f}, RMS={level:.1f}, ratio={ratio:.2f}")
            if fundamental < 10.0 or level < 5.0 or ratio < 0.08 or peak > 32000:
                raise SystemExit(f"sample piano note {note}: invalid pitch, level, or peak")
            sample_renders[key] = audio

        synth_level = rms(synth[int(0.15 * RATE) : int(0.35 * RATE)])
        sample_level = rms(sample_renders[13][int(0.15 * RATE) : int(0.35 * RATE)])
        balance = sample_level / synth_level if synth_level else 0.0
        print(f"sample-to-synth midrange RMS ratio={balance:.2f}")
        if not 0.45 <= balance <= 2.0:
            raise SystemExit("sampled piano level is not comparable to the additive piano")

        synth_soft = render(host, directory, "piano_synth_soft", 13, 20, False)
        synth_soft_level = rms(synth_soft[int(0.15 * RATE) : int(0.35 * RATE)])
        synth_velocity_ratio = synth_level / synth_soft_level if synth_soft_level else 0.0
        print(f"additive piano velocity 20→100 RMS ratio={synth_velocity_ratio:.2f}")
        if synth_velocity_ratio < 1.5:
            raise SystemExit("the additive piano A/B baseline lost velocity response")

        soft = render(host, directory, "piano_sample_soft", 13, 20, True)
        loud = render(host, directory, "piano_sample_loud", 13, 100, True)
        soft_level = rms(soft[int(0.15 * RATE) : int(0.35 * RATE)])
        loud_level = rms(loud[int(0.15 * RATE) : int(0.35 * RATE)])
        velocity_ratio = loud_level / soft_level if soft_level else 0.0
        print(f"sample piano velocity 20→100 RMS ratio={velocity_ratio:.2f}")
        if velocity_ratio < 1.5:
            raise SystemExit("sampled piano velocity response was not measurable")

        released = sample_renders[13]
        active_tail = rms(released[int(0.43 * RATE) : int(0.45 * RATE)])
        release_tail = rms(released[int(0.59 * RATE) : int(0.60 * RATE)])
        release_ratio = release_tail / active_tail if active_tail else 0.0
        print(f"sample piano post-release RMS ratio={release_ratio:.3f}")
        if release_ratio > 0.05:
            raise SystemExit("sampled piano note-off did not fade cleanly")

        chord = render_chord(host, directory)
        chord_peak = max(abs(sample) for sample in chord)
        print(f"sample piano six-voice chord peak={chord_peak:.0f}")
        if chord_peak > 32000:
            raise SystemExit("sampled piano six-voice chord clipped")


if __name__ == "__main__":
    main()
