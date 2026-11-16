#!/usr/bin/env python3
"""Pack bass samples as 8-bit mono."""
import argparse
import math
import wave
from pathlib import Path


SAMPLE_RATE = 44100
SAMPLES = (
    ("bass_nomore", "BASS_NOMORE_LENGTH", "NoMoreBass.wav"),
    ("bass_808", "BASS_808_LENGTH", "Bass808.wav"),
    ("bass_plunk1", "BASS_PLUNK1_LENGTH", "BassPlunk1.wav"),
    ("bass_plunk3", "BASS_PLUNK3_LENGTH", "BassPlunk3.wav"),
    ("bass_plunk4", "BASS_PLUNK4_LENGTH", "BassPlunk4.wav"),
)


def read_sample(path):
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        width = wav.getsampwidth()
        if (
            wav.getcomptype() != "NONE"
            or channels not in (1, 2)
            or width not in (2, 3)
            or wav.getframerate() not in (44100, 48000)
        ):
            raise ValueError(f"{path}: expected uncompressed 44.1 or 48 kHz, mono or stereo, 16- or 24-bit PCM")
        source_rate = wav.getframerate()
        raw = wav.readframes(wav.getnframes())

    frame_width = channels * width
    scale = 1 << (8 * (width - 1))
    samples = []
    for offset in range(0, len(raw), frame_width):
        frame = [
            int.from_bytes(raw[offset + channel * width : offset + (channel + 1) * width],
                           "little", signed=True)
            for channel in range(channels)
        ]
        samples.append(sum(frame) / channels / scale)
    if not samples:
        raise ValueError(f"{path}: sample is empty")
    if source_rate != SAMPLE_RATE:
        # Linear resample so pitch matches the 44.1 kHz playback clock.
        count = int(len(samples) * SAMPLE_RATE / source_rate)
        step = source_rate / SAMPLE_RATE
        resampled = []
        for i in range(count):
            position = i * step
            index = int(position)
            nxt = samples[min(index + 1, len(samples) - 1)]
            resampled.append(samples[index] + (nxt - samples[index]) * (position - index))
        samples = resampled
    return [max(-128, min(127, int(round(value)))) for value in samples]


def low_pass(samples, cutoff):
    k = math.tan(math.pi * cutoff / SAMPLE_RATE)
    norm = 1.0 / (1.0 + math.sqrt(2.0) * k + k * k)
    b0 = k * k * norm
    b1 = 2.0 * b0
    b2 = b0
    a1 = 2.0 * (k * k - 1.0) * norm
    a2 = (1.0 - math.sqrt(2.0) * k + k * k) * norm
    x1 = x2 = y1 = y2 = 0.0
    filtered = []
    for sample in samples:
        value = b0 * sample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        filtered.append(value)
        x2, x1 = x1, sample
        y2, y1 = y1, value
    return filtered


def match_rms(samples, reference):
    source_rms = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
    reference_rms = math.sqrt(sum(sample * sample for sample in reference) / len(reference))
    gain = reference_rms / source_rms if source_rms else 1.0
    return [max(-128, min(127, int(round(sample * gain)))) for sample in samples]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample_dir", type=Path, help="directory containing the bass WAV files")
    parser.add_argument("--output", type=Path, default=Path("dsp/bass_samples.h"))
    args = parser.parse_args()

    samples = {
        symbol: read_sample(args.sample_dir / filename)
        for symbol, _, filename in SAMPLES
    }
    lines = [
        "/* Generated from configured bass samples; do not edit. */",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for symbol, length, _ in SAMPLES:
        values = samples[symbol]
        lines.append(f"static const int8_t {symbol}[] = {{")
        for offset in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(value) for value in values[offset : offset + 16]) + ",")
        lines.append("};")
        lines.append(f"#define {length} ((uint32_t)(sizeof {symbol} / sizeof {symbol}[0]))")
        lines.append("")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
