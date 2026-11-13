"""Print harmonic balance, envelope and brightness of a rendered note (mono mix of a WAV)."""
import sys
import numpy as np
from scipy.io import wavfile


def analyse(path, f_guess, start=0.0):
    sr, data = wavfile.read(path)
    x = data.astype(np.float64)
    if x.ndim > 1:
        x = x.mean(1)
    x /= max(1.0, np.abs(x).max())
    on = np.argmax(np.abs(x) > 0.002) / sr
    t0 = on + start
    env = [20 * np.log10(np.sqrt((x[int((on + i * 0.05) * sr):int((on + i * 0.05 + 0.05) * sr)] ** 2).mean()) + 1e-9)
           for i in range(0, 30)]
    print(f"{path}: onset {on:.2f}s, envelope dB per 50 ms:", " ".join(f"{v:.0f}" for v in env[:24]))
    n = 1 << 15
    seg = x[int((t0 + 0.4) * sr):int((t0 + 0.4) * sr) + n] * np.hanning(n)
    spec = np.abs(np.fft.rfft(seg))
    f = np.fft.rfftfreq(n, 1 / sr)
    lo = (f > f_guess * 0.9) & (f < f_guess * 1.1)
    f0 = f[np.argmax(spec * lo)]
    amps = []
    for h in range(1, 13):
        m = (f > f0 * h * 0.97) & (f < f0 * h * 1.03)
        amps.append(20 * np.log10(spec[m].max() / spec[np.argmax(spec * lo)] + 1e-9))
    print(f"  f0 {f0:.1f} Hz harmonics dB:", " ".join(f"{a:.0f}" for a in amps))
    cent = (f * spec ** 2).sum() / (spec ** 2).sum()
    print(f"  centroid {cent:.0f} Hz, peak {20 * np.log10(np.abs(x).max()):.1f} dB (normalised)")


if __name__ == "__main__":
    analyse(sys.argv[1], float(sys.argv[2]))
