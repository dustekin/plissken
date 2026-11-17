"""Verify each piano and synth control changes live audio."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


def samples(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 2 or wav.getframerate() != 44100:
            raise ValueError(f"{path}: expected 44.1 kHz 16-bit stereo audio")
        data = wav.readframes(wav.getnframes())
    return struct.unpack("<" + "h" * (len(data) // 2), data)[::2]


def render(host, directory, track, knob, enabled):
    setting = 3 if enabled else -3
    script = (f"wait 2000\nexpect solo{track} {1 << track}\n"
              f"turn SELECT {track - 1}\nturn K{knob} {setting}\n"
              f"expect fx{knob} {3 if enabled else 0}\n")
    script += "wav audio.wav\nkey 7 down 100\nwait 1000\nkey 7 up\nwait 1000\nwavstop\n"
    script_path = directory / "test.txt"
    script_path.write_text(script)
    subprocess.run([str(host), str(script_path), str(directory)], check=True, stdout=subprocess.DEVNULL)
    return samples(directory / "audio.wav")


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    with tempfile.TemporaryDirectory(prefix="plissken-voice-controls-") as tmp:
        directory = Path(tmp)
        for track in (0, 1, 2, 6):
            for knob in range(1, 5):
                dry = render(host, directory, track, knob, False)
                wet = render(host, directory, track, knob, True)
                if len(dry) != len(wet) or not dry:
                    raise SystemExit(f"track {track} K{knob}: invalid output length")
                energy = sum(x * x for x in dry)
                if not energy:
                    raise SystemExit(f"track {track} K{knob}: silent dry output")
                difference = math.sqrt(sum((a - b) ** 2 for a, b in zip(dry, wet)) / energy)
                print(f"track {track} K{knob} live: {difference:.4f}")
                if difference < 0.001:
                    raise SystemExit(f"track {track} K{knob}: no measurable effect")


if __name__ == "__main__":
    main()
