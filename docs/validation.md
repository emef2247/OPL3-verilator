# Validation — opl3_sim vs Nuked OPL3

Method: render the same OPL3 VGM stream with `opl3_sim` (this repo)
and with a Nuked OPL3 harness (gain 1.0), align by lag search, and
compare level, waveform correlation, and per-harmonic levels.
Measured 2026-11-28, Verilator 5.020, core @ 491a4dc with the two
documented configuration constants.

## Divider check (a harness bug found and fixed)

With the upstream `$ceil` divider expression kept, the core ran at
14.31818 MHz / 289 (0.35% slow): levels matched Nuked (RMS ratio
1.002) but waveforms decorrelated (corr ≈ 0.05). `CLK_DIV_COUNT` is
therefore pinned to exactly 288 in `rtl/opl3_pkg.sv`. After the fix,
the native sample count for a 36.8 s stream matches the expected
count (1,834,523 captured vs 1,834,488 expected including the 0.1 s
tail).

## Single sustained tones (0.5 s probes, A4)

- 2-op (sine, modulator TL 24): steady-state correlation **0.976**,
  RMS identical (3271 vs 3272), harmonics H1–H8 within **0.4 dB**.
- 4-op (pair ch0/ch3, algorithm 0): steady-state correlation
  **0.238** with RMS identical (2527 vs 2527) and harmonics within
  ~1.7 dB (worst at H5). Same spectrum, different waveform: the two
  models' 4-op phase/topology behaviour differs materially.

## Full song (SALAMD02 OPL3 stem, 36.8 s, SCC-converted 4-op patches)

- Lengths identical (1,622,886 frames), RMS ratio RTL/Nuked **1.001**.
- Waveform correlation at best lag (−21 samples): **0.79** in the
  10–15 s window, **0.67** full-length; per-4 s blocks range
  0.29–0.81, lowest late in the song.

## Reading

The models agree on the 2-op basics and on overall level, and
diverge in 4-op waveform structure. Which one matches the real
YMF262 in 4-op operation cannot be settled by comparing the two
models alone; a real-hardware recording (e.g. MoonSound FM, which is
the OPL3 block) of the 4-op probe is the arbiter.
