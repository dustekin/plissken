"""Render the embedded Escape from New York MIDI song through Plissken's host engine."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import wave


def main():
    host = Path(sys.argv[1] if len(sys.argv) > 1 else "build/host/plissken_host").resolve()
    output = Path(sys.argv[2] if len(sys.argv) > 2 else "renders/escape-from-new-york.wav").resolve()
    test = Path(__file__).resolve().parents[1] / "tests" / "song.txt"
    if not host.is_file():
        raise SystemExit(f"Host renderer not found: {host}")
    output.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="plissken-escape-render-") as tmp:
        directory = Path(tmp)
        result = subprocess.run(
            [str(host), str(test), str(directory)],
            text=True,
            capture_output=True,
        )
        if result.returncode:
            raise RuntimeError(f"Escape from New York MIDI render failed:\n{result.stdout}{result.stderr}")
        source = directory / "song.wav"
        if not source.is_file():
            raise FileNotFoundError(f"Host test did not produce the expected WAV: {source}")
        with wave.open(str(source), "rb") as wav:
            if (wav.getframerate(), wav.getnchannels(), wav.getsampwidth()) != (44100, 2, 2):
                raise ValueError("Expected 44.1 kHz, 16-bit stereo audio")
            frames = wav.getnframes()
            raw = wav.readframes(frames)
        samples = struct.unpack("<" + "h" * (len(raw) // 2), raw)
        if frames == 0 or max(abs(sample) for sample in samples) == 0:
            raise ValueError("Escape from New York render is silent")
        shutil.copyfile(source, output)

    print(f"Saved {output}")
    print(f"Duration: {frames / 44100:.2f}s; peak: {max(abs(sample) for sample in samples)}")
    print(result.stdout.strip())


if __name__ == "__main__":
    main()
