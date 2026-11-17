"""Render one sustained F-major chord WAV for each CS-80-inspired patch."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import wave


PATCHES = (
    "tyrell",
    "raindrops",
    "africa",
    "br_blues",
    "br_strings",
    "rosanna",
    "billie_jean",
    "end_titles",
    "blade_brass",
    "chariots",
)
RATE = 44100
SCRIPT_TEMPLATE = """\
wait 2000
turn SELECT 5
expect instrument 6
turn PRESET {patch}
expect cs80_preset {patch}
wav {filename}
key 0 down 100 0
key 4 down 100 0
key 7 down 100 0
wait 900
key 0 up
key 4 up
key 7 up
wait 900
wavstop
"""


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    output = Path(sys.argv[2] if len(sys.argv) > 2 else "renders/cs80-chords").resolve()
    if not host.is_file():
        raise SystemExit(f"Host renderer not found: {host}")
    output.mkdir(parents=True, exist_ok=True)

    for patch, name in enumerate(PATCHES, 1):
        filename = f"{patch:02d}_{name}_f_major.wav"
        scenario = output / f".{name}.txt"
        scenario.write_text(SCRIPT_TEMPLATE.format(patch=patch, filename=filename))
        try:
            result = subprocess.run(
                [str(host), str(scenario), str(output)],
                check=False,
                text=True,
                capture_output=True,
            )
            if result.returncode:
                raise RuntimeError(f"Host render failed for {name}:\n{result.stdout}{result.stderr}")
        finally:
            scenario.unlink(missing_ok=True)

        path = output / filename
        with wave.open(str(path), "rb") as wav:
            if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (RATE, 2, 2):
                raise ValueError(f"{path}: expected 44.1 kHz, 16-bit stereo audio")
            raw = wav.readframes(wav.getnframes())
        samples = struct.unpack("<" + "h" * (len(raw) // 2), raw)
        if not samples or max(abs(sample) for sample in samples) == 0:
            raise ValueError(f"{path}: rendered audio is silent")
        rms = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
        peak = max(abs(sample) for sample in samples)
        print(f"{path.name}: {wav.getnframes() / RATE:.2f}s, RMS {rms:.0f}, peak {peak}")


if __name__ == "__main__":
    main()
