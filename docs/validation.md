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

## Reading (model comparison only)

The models agree on the 2-op basics and on overall level, and
diverge in 4-op waveform structure. Which one matches the real
YMF262 in 4-op operation cannot be settled by comparing the two
models alone; a real-hardware recording of the 4-op probe is the
arbiter.

## Real-hardware adjudication (2026-11-28, owner's recordings)

The owner played the two probe VGMs (2-op and 4-op algorithm 0,
sustained A4) and the SALAMD01–14 opl3-profile variant VGMs on his
MSX hardware and recorded the outputs (48 kHz stereo, dual-mono).

- 2-op probe: the recording's harmonic profile matches both models
  (H1–H7 within ~1 dB). Control passes.
- 4-op probe: the recording matches **Nuked**, not this RTL.
  Per-harmonic error of the recording against each render
  (mean |Δ| over H2–H10, two steady windows): Nuked **0.64 dB**
  (max 1.6 dB); this RTL **6.5 dB** (max 15.6 dB — H2, H3 and H6
  are 7–16 dB too weak). The sustain level is also consistent with
  Nuked once the session gain measured on the 2-op probe (~1.1×)
  is applied.
- The full-song recordings could not separate the models
  (waveform and STFT comparisons tie; the recording chain smears
  phase), which is why the single-note probes were needed.

Conclusion: for 4-op operation this RTL's output diverges from the
real chip, and Nuked reproduces the chip. Per this repo's policy
the difference is recorded here as a finding; the vendored core is
not patched. Downstream, Nuked remains the reference renderer for
OPL3 fitting work, and this simulator should not be used as the
hardware reference for 4-op patches.
