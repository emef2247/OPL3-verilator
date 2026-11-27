# OPL3-verilator

Render OPL3 (YMF262) VGM files to stereo WAV with Greg Taylor's OPL3
FPGA RTL core under Verilator.

The RTL in `rtl/` is Greg Taylor's
[opl3_fpga](https://github.com/gtaylormb/opl3_fpga) (commit 491a4dc),
a reverse-engineered SystemVerilog implementation of the Yamaha
YMF262. It is **not** an original core of this repository: it is
vendored unmodified except for one configuration constant in
`rtl/opl3_pkg.sv`, where `CLK_FREQ` is set to the genuine YMF262
master clock (14.31818 MHz, marked with an `OPL3-verilator:` comment)
so the sample divider is exactly 288 and the output sample rate is
exactly 49715.97 Hz.

The C++ harness in `src/` only decodes VGM, drives the core's host
bus (address/data writes for the 0x5E/0x5F commands), captures the
core's stereo sample stream, and resamples it to 44.1 kHz.

## License

LGPLv3, following the FPGA core. See `LICENSE` (the upstream
`COPYING.LESSER`, with the GPLv3 text it incorporates).

## Build

```
./build.sh        # -> build/obj_dir/opl3_sim
```

Requires Verilator (developed against 5.020).

## Usage

```
opl3_sim INPUT.vgm --output FILE.wav [--max-seconds N]
```

Only OPL3 writes (0x5E/0x5F) are driven to the core; other VGM
commands are skipped with their correct lengths, so a mixed-chip VGM
renders its OPL3 part only. Output is 16-bit stereo WAV at 44.1 kHz,
length from the VGM header's total-samples field.

## Validation

The harness was cross-checked against Nuked OPL3 renders of the same
VGM streams (waveform correlation, level, per-harmonic snapshot).
See `docs/validation.md`.
