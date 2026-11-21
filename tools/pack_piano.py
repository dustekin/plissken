#!/usr/bin/env python3
"""Encode CC0 piano WAVs as 4-bit IMA ADPCM firmware samples."""
import argparse
from pathlib import Path

from ima_adpcm import encode_sample

SAMPLES = (
    ("piano_c2", "PIANO_C2_LENGTH", "piano_c2.wav", 48),
    ("piano_fs2", "PIANO_FS2_LENGTH", "piano_fs2.wav", 54),
    ("piano_c3", "PIANO_C3_LENGTH", "piano_c3.wav", 60),
    ("piano_fs3", "PIANO_FS3_LENGTH", "piano_fs3.wav", 66),
    ("piano_c4", "PIANO_C4_LENGTH", "piano_c4.wav", 72),
    ("piano_fs4", "PIANO_FS4_LENGTH", "piano_fs4.wav", 78),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample_dir", type=Path, help="directory containing the CC0 piano WAV files")
    parser.add_argument("--output", type=Path, default=Path("dsp/piano_samples.h"))
    args = parser.parse_args()

    samples = {
        symbol: encode_sample(args.sample_dir / filename)
        for symbol, _, filename, _ in SAMPLES
    }
    lines = [
        "/* Generated as 4-bit IMA ADPCM from the CC0 piano samples; see CREDITS.md. */",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for symbol, length, _, _ in SAMPLES:
        values, frames = samples[symbol]
        lines.append(f"static const uint8_t {symbol}[] = {{")
        for offset in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(value) for value in values[offset : offset + 16]) + ",")
        lines.append("};")
        lines.append(f"#define {length} {frames}u")
        lines.append("")

    lines.append(f"#define PIANO_SAMPLE_COUNT {len(SAMPLES)}u")
    lines.append("static const int8_t piano_sample_roots[PIANO_SAMPLE_COUNT] = {")
    lines.append("    " + ", ".join(str(root) for _, _, _, root in SAMPLES))
    lines.append("};")
    lines.append("static const uint8_t *const piano_sample_data[PIANO_SAMPLE_COUNT] = {")
    lines.append("    " + ", ".join(symbol for symbol, _, _, _ in SAMPLES))
    lines.append("};")
    lines.append("static const uint32_t piano_sample_lengths[PIANO_SAMPLE_COUNT] = {")
    lines.append("    " + ", ".join(length for _, length, _, _ in SAMPLES))
    lines.append("};")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    print(f"Wrote {args.output} ({sum(len(values) for values, _ in samples.values())} ADPCM bytes)")


if __name__ == "__main__":
    main()
