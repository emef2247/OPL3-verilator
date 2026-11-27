# AGENTS.md — OPL3-verilator

- Purpose: render OPL3 (YMF262) VGM files to WAV with Greg Taylor's
  OPL3 FPGA RTL (gtaylormb/opl3_fpga) under Verilator. The RTL
  generates the sound; the C++ harness only decodes VGM, drives
  clocks/writes, and captures output.
- The RTL is third-party (LGPLv3) and vendored: keep it byte-identical
  to upstream except the single documented `CLK_FREQ` configuration
  change in `rtl/opl3_pkg.sv`. Do not patch core behaviour here; a
  behavioural difference against other renderers is a finding to
  report, not something to tune away in the vendored copy.
- Build: `./build.sh` → `build/obj_dir/opl3_sim`.
- Validate changes against Nuked OPL3 renders of the same VGM stream
  (correlation, level, harmonics) before claiming behaviour; final
  judgement of OPL3 sound is the user's, on real hardware where
  possible.
