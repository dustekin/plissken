#!/usr/bin/env python3
"""Pack retained owner bass samples and CC0 bass notes for firmware."""
import argparse
import wave
from pathlib import Path

from ima_adpcm import encode_sample


SAMPLE_RATE = 44100
OWNER_SAMPLES = (
    ("bass_nomore", "BASS_NOMORE_LENGTH", "NoMoreBass.wav"),
    ("bass_808", "BASS_808_LENGTH", "Bass808.wav"),
)
CC0_SAMPLES = (
    ("bass_electric_a_sharp3", "BASS_ELECTRIC_LENGTH", "bass_electric_a_sharp3.wav", 58),
    ("bass_acoustic_c2", "BASS_ACOUSTIC_LENGTH", "bass_acoustic_c2.wav", 36),
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample_dir", type=Path, help="directory containing the owner bass WAV files")
    parser.add_argument("--output", type=Path, default=Path("dsp/bass_samples.h"))
    args = parser.parse_args()

    owner_samples = {
        symbol: read_sample(args.sample_dir / filename)
        for symbol, _, filename in OWNER_SAMPLES
    }
    cc0_samples = {
        symbol: encode_sample(args.sample_dir / "bass-cc0" / filename)
        for symbol, _, filename, _ in CC0_SAMPLES
    }
    lines = [
        "/* Generated from retained owner and CC0 bass samples; see CREDITS.md. */",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for symbol, length, _ in OWNER_SAMPLES:
        values = owner_samples[symbol]
        lines.append(f"static const int8_t {symbol}[] = {{")
        for offset in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(value) for value in values[offset : offset + 16]) + ",")
        lines.append("};")
        lines.append(f"#define {length} ((uint32_t)(sizeof {symbol} / sizeof {symbol}[0]))")
        lines.append("")

    for symbol, length, _, _ in CC0_SAMPLES:
        values, frames = cc0_samples[symbol]
        lines.append(f"static const uint8_t {symbol}[] = {{")
        for offset in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(value) for value in values[offset : offset + 16]) + ",")
        lines.append("};")
        lines.append(f"#define {length} {frames}u")
        lines.append("")

    lines.append(f"#define BASS_CC0_SAMPLE_COUNT {len(CC0_SAMPLES)}u")
    lines.append("static const int8_t bass_cc0_roots[BASS_CC0_SAMPLE_COUNT] = {")
    lines.append("    " + ", ".join(str(root) for _, _, _, root in CC0_SAMPLES))
    lines.append("};")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    pcm_bytes = sum(len(values) for values in owner_samples.values())
    adpcm_bytes = sum(len(values) for values, _ in cc0_samples.values())
    print(f"Wrote {args.output} ({pcm_bytes} PCM bytes, {adpcm_bytes} ADPCM bytes)")


if __name__ == "__main__":
    main()
