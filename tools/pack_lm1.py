#!/usr/bin/env python3
"""Convert the Rossf Linn LM-1 WAVs into 8-bit mono firmware samples."""
import argparse
import wave
from pathlib import Path


SAMPLES = (
    ("rossflm1_kick", "ROSSFLM1_KICK_LENGTH", "rossflm1_kick.wav"),
    ("rossflm1_snare", "ROSSFLM1_SNARE_LENGTH", "rossflm1_snare.wav"),
    ("rossflm1_hat_closed", "ROSSFLM1_HAT_CLOSED_LENGTH", "rossflm1_hat-closed.wav"),
    ("rossflm1_hat_open", "ROSSFLM1_HAT_OPEN_LENGTH", "rossflm1_hat-open.wav"),
    ("rossflm1_tom_hi", "ROSSFLM1_TOM_HI_LENGTH", "rossflm1_tom-hi.wav"),
    ("rossflm1_tom_lo", "ROSSFLM1_TOM_LO_LENGTH", "rossflm1_tom-lo.wav"),
    ("rossflm1_clap", "ROSSFLM1_CLAP_LENGTH", "rossflm1_clap.wav"),
    ("rossflm1_cowbell", "ROSSFLM1_COWBELL_LENGTH", "rossflm1_cowbell.wav"),
    ("rossflm1_tambourine", "ROSSFLM1_TAMBOURINE_LENGTH", "rossflm1_tambourine.wav"),
    ("rossflm1_cabasa", "ROSSFLM1_CABASA_LENGTH", "rossflm1_cabasa.wav"),
    ("rossflm1_clave", "ROSSFLM1_CLAVE_LENGTH", "rossflm1_clave.wav"),
    ("rossflm1_conga_hi", "ROSSFLM1_CONGA_HI_LENGTH", "rossflm1_conga-hi.wav"),
    ("rossflm1_conga_lo", "ROSSFLM1_CONGA_LO_LENGTH", "rossflm1_conga-lo.wav"),
)


def read_sample(path):
    with wave.open(str(path), "rb") as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (
            1,
            2,
            44100,
            "NONE",
        ):
            raise ValueError(f"{path}: expected uncompressed 44.1 kHz, mono, 16-bit PCM")
        raw = wav.readframes(wav.getnframes())

    samples = []
    scale = 1 << 8
    for offset in range(0, len(raw), 2):
        value = int.from_bytes(raw[offset : offset + 2], "little", signed=True)
        quantized = (
            (value + scale // 2) // scale if value >= 0
            else -((-value + scale // 2) // scale)
        )
        samples.append(max(-128, min(127, quantized)))
    if not samples:
        raise ValueError(f"{path}: sample is empty")
    return samples


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample_dir", type=Path, help="directory containing the Rossf LM-1 WAVs")
    parser.add_argument(
        "--output", type=Path, default=Path("dsp/lm1_samples.h"), help="generated header path"
    )
    args = parser.parse_args()

    lines = [
        "/* Generated from Rossf's Linn LM-1 pack; see project credits and license. */",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for symbol, length, filename in SAMPLES:
        values = read_sample(args.sample_dir / filename)
        lines.append(f"static const int8_t {symbol}[] = {{")
        for offset in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(value) for value in values[offset : offset + 16]) + ",")
        lines.append("};")
        lines.append(f"#define {length} ((uint32_t)(sizeof {symbol} / sizeof {symbol}[0]))")
        lines.append("")
    lines.append(f"#define ROSSFLM1_SAMPLE_COUNT {len(SAMPLES)}u")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
