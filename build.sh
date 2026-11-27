#!/bin/bash
# Build the opl3_sim Verilator harness.
set -e
cd "$(dirname "$0")"
mkdir -p build
verilator --cc --exe --build -Wno-fatal -O2 -CFLAGS "-O2" -sv \
    --top-module opl3 --Mdir build/obj_dir -o opl3_sim \
    "$PWD/rtl/opl3_pkg.sv" "$PWD/rtl/afifo.v" \
    $(ls "$PWD"/rtl/*.sv | grep -v opl3_pkg) \
    "$PWD/src/main.cpp"
echo "built build/obj_dir/opl3_sim"
