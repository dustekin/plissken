"""Check that each drum effect changes a repeatable isolated kick render."""
import math
from pathlib import Path
import struct
import sys
import wave


def samples(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 2 or wav.getframerate() != 44100:
            raise ValueError(f"{path}: expected 44.1 kHz 16-bit stereo audio")
        data = wav.readframes(wav.getnframes())
    return struct.unpack("<" + "h" * (len(data) // 2), data)[::2]


def main():
    directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/host")
    dry = samples(directory / "drum_fx_dry.wav")
    if not dry:
        raise SystemExit("empty dry render")
    energy = sum(x * x for x in dry)
    if not energy:
        raise SystemExit("silent dry render")
    for name in ("chorus", "reverb", "compression"):
        wet = samples(directory / f"drum_fx_{name}.wav")
        if len(wet) != len(dry):
            raise SystemExit(f"{name}: render length differs")
        ratio = math.sqrt(sum((a - b) ** 2 for a, b in zip(dry, wet)) / energy)
        print(f"{name}: difference RMS / dry RMS = {ratio:.3f}")
        if ratio < 0.02:
            raise SystemExit(f"{name}: effect did not measurably change the render")


if __name__ == "__main__":
    main()
