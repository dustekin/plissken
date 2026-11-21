#!/usr/bin/env python3
"""Pack CC0 single-note guitar samples as 4-bit IMA ADPCM."""
import argparse
from pathlib import Path

from ima_adpcm import encode_sample


SAMPLES = (
    ("guitar_electric_g2", "GUITAR_ELECTRIC_G2_LENGTH", "guitar_electric_g2.wav", 43),
    ("guitar_nylon_g3", "GUITAR_NYLON_G3_LENGTH", "guitar_nylon_g3.wav", 55),
    ("guitar_acoustic_e2", "GUITAR_ACOUSTIC_E2_LENGTH", "guitar_acoustic_e2.wav", 40),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample_dir", type=Path, help="directory containing the guitar WAV files")
    parser.add_argument("--output", type=Path, default=Path("dsp/guitar_samples.h"))
    args = parser.parse_args()

    samples = {
        symbol: encode_sample(args.sample_dir / filename)
        for symbol, _, filename, _ in SAMPLES
    }
    lines = [
        "/* Generated as 4-bit IMA ADPCM from the processed CC0 guitar samples; see CREDITS.md. */",
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

    lines.append(f"#define GUITAR_SAMPLE_COUNT {len(SAMPLES)}u")
    lines.append("static const int8_t guitar_sample_roots[GUITAR_SAMPLE_COUNT] = {")
    lines.append("    " + ", ".join(str(root) for _, _, _, root in SAMPLES))
    lines.append("};")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    print(f"Wrote {args.output} ({sum(len(values) for values, _ in samples.values())} ADPCM bytes)")


if __name__ == "__main__":
    main()
