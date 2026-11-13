"""Verify that live note velocity changes synthesized output level."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


def rms(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 2 or wav.getframerate() != 44100:
            raise ValueError(f"{path}: expected 44.1 kHz 16-bit stereo audio")
        raw = wav.readframes(wav.getnframes())
    samples = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    if not samples:
        raise ValueError(f"{path}: no samples")
    return (sum(sample * sample for sample in samples) / len(samples)) ** 0.5


def render(host, directory, velocity):
    script = (f"wait 2000\nturn SELECT -1\nwav velocity_{velocity}.wav\n"
              f"key 7 down {velocity} 0\nwait 500\nkey 7 up\nwait 100\nwavstop\n")
    scenario = directory / f"velocity_{velocity}.txt"
    scenario.write_text(script)
    subprocess.run([str(host), str(scenario), str(directory)], check=True, stdout=subprocess.DEVNULL)
    return rms(directory / f"velocity_{velocity}.wav")


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-velocity-") as tmp:
        directory = Path(tmp)
        soft = render(host, directory, 20)
        loud = render(host, directory, 120)
        ratio = loud / soft if soft else 0.0
        print(f"Juno live RMS: velocity 20={soft:.1f}, 120={loud:.1f}, ratio={ratio:.2f}")
        if soft <= 0.0 or ratio < 1.5:
            raise SystemExit("live velocity response was not measurable")


if __name__ == "__main__":
    main()
