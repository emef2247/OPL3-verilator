"""Compare the RTL (gtaylormb/opl3 via opl3_sim) and Nuked renders of
the same OPL3 VGM: length, level, lag-aligned correlation, and a
per-harmonic snapshot of a stable segment."""
import sys
import wave
import numpy as np


def read_wav(path):
    with wave.open(path) as w:
        rate, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
        raw = w.readframes(n)
    a = np.frombuffer(raw, dtype='<i2').astype(float) / 32768.0
    return a.reshape(-1, ch), rate


rtl, r1 = read_wav(sys.argv[1])
nuk, r2 = read_wav(sys.argv[2])
assert r1 == r2 == 44100, (r1, r2)
print('rtl frames', len(rtl), 'nuked frames', len(nuk))
n = min(len(rtl), len(nuk))
rtl, nuk = rtl[:n], nuk[:n]
for ch, name in ((0, 'L'), (1, 'R')):
    a, b = rtl[:, ch], nuk[:, ch]
    print(f'{name}: rtl rms {np.sqrt((a**2).mean()):.4f} peak {np.abs(a).max():.3f} | '
          f'nuked rms {np.sqrt((b**2).mean()):.4f} peak {np.abs(b).max():.3f}')
    # lag search on a loud 5 s window in the middle
    s = slice(10 * 44100, 15 * 44100)
    aw, bw = a[s], b[s]
    best = (0, -2.0)
    for lag in range(-300, 301):
        if lag >= 0:
            x, y = aw[lag:], bw[:len(bw) - lag]
        else:
            x, y = aw[:lag], bw[-lag:]
        c = np.corrcoef(x, y)[0, 1]
        if c > best[1]:
            best = (lag, c)
    print(f'{name}: best lag {best[0]} samples, corr {best[1]:.4f}')
    # full-length correlation at that lag
    lag = best[0]
    if lag >= 0:
        x, y = a[lag:], b[:len(b) - lag]
    else:
        x, y = a[:lag], b[-lag:]
    print(f'{name}: full corr at lag {lag}: {np.corrcoef(x, y)[0, 1]:.4f}, '
          f'rms ratio rtl/nuked {np.sqrt((x**2).mean()) / np.sqrt((y**2).mean()):.3f}')
