"""Verify low-frequency energy increases in live and sequencer drum renders."""
import math
from pathlib import Path
import struct
import sys
import wave


def low_rms(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 2 or wav.getframerate() != 44100:
            raise ValueError(f"{path}: expected 44.1 kHz 16-bit stereo audio")
        data = wav.readframes(wav.getnframes())
    samples = struct.unpack("<" + "h" * (len(data) // 2), data)[::2]
    if not samples:
        raise ValueError(f"{path}: empty audio")
    low = energy = 0.0
    for sample in samples:
        low += 0.0183 * (sample - low)
        energy += low * low
    return math.sqrt(energy / len(samples))


def main():
    directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/host")
    for prefix in ("drum_boost", "drum_sequence"):
        off = low_rms(directory / f"{prefix}_off.wav")
        on = low_rms(directory / f"{prefix}_on.wav")
        print(f"{prefix}: low-band RMS off={off:.2f} on={on:.2f}")
        if not on > off * 1.2:
            raise SystemExit(f"{prefix}: insufficient bass boost")


if __name__ == "__main__":
    main()
