#!/usr/bin/env bash
# Runs the UNO firmware on a simulated ATmega328P with a virtual HC-SR04 and two virtual servos.
# Usage: uno/sim/run.sh            (builds what is missing, then prints the virtual serial port)
# Change the water distance while it runs: echo 150 > uno/sim/.build/distance_mm  (-1 = no echo)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="$HERE/.build"
SIMAVR_TAG="v1.7"
mkdir -p "$BUILD"
AVR_GCC="$(ls -d "$HOME"/.arduino15/packages/arduino/tools/avr-gcc/*/ 2>/dev/null | head -1 || true)"
if [ -z "$AVR_GCC" ]; then
  AVR_GCC="$HOME/.platformio/packages/toolchain-atmelavr/"
fi
if [ ! -f "$BUILD/simavr/simavr/obj-$(gcc -dumpmachine)/libsimavr.a" ]; then
  git -c advice.detachedHead=false clone -q --depth 1 --branch "$SIMAVR_TAG" https://github.com/buserror/simavr.git "$BUILD/simavr"
  make -s -C "$BUILD/simavr/simavr" AVR_ROOT="$AVR_GCC" AVR="${AVR_GCC}bin/avr-" RELEASE=1 >/dev/null
fi
OBJ="$BUILD/simavr/simavr/obj-$(gcc -dumpmachine)"
gcc -O2 -Wall -o "$BUILD/uno_sim" "$HERE/uno_sim.c" -I"$BUILD/simavr/simavr/sim" "$OBJ/libsimavr.a" -lelf -lm
(cd "$HERE/.." && pio run -s)
[ -f "$BUILD/distance_mm" ] || echo 200 > "$BUILD/distance_mm"
exec "$BUILD/uno_sim" "$HERE/../.pio/build/uno/firmware.elf" "$BUILD/distance_mm" "$BUILD/ttyUNO"
