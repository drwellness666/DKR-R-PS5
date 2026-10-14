#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Host semantic tests; no console, renderer or cartridge required.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: test_native_adapters.sh ABSOLUTE_GPU_WORK ABSOLUTE_TEST_WORK' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
gpu=$1
test_work=$2
[[ $gpu == /* && $test_work == /* ]] || exit 2
mkdir -p "$test_work"
clang -std=c11 -D_GNU_SOURCE -Dtimegm=dkrr_timegm -fsanitize=address,undefined -c "$root/src/ps5_timegm.c" -o "$test_work/timegm.o"
clang -std=c11 -fsanitize=address,undefined "$root/tests/native_timegm.c" "$test_work/timegm.o" -o "$test_work/timegm-test"
ASAN_OPTIONS=detect_leaks=0 "$test_work/timegm-test"
python3 - "$gpu/PS5_PayloadSDK/platform/src/cxa.c" "$test_work/cxa.c" <<'PY'
from pathlib import Path
import hashlib,sys
data=Path(sys.argv[1]).read_bytes()
if hashlib.sha256(data).hexdigest()!='93c6893242c352b9128a272f21b5aa005635aa38107771acfc79ac2d5169d872':
    raise SystemExit('Unreviewed SDK destructor shim')
data=data.replace(b'#include "ps5platform/libc.h"',b'#include <stdbool.h>')
data=data.replace(b'ps5___cxa_thread_atexit_impl',b'__cxa_thread_atexit_impl')
Path(sys.argv[2]).write_bytes(data)
PY
python3 "$root/tools/prepare_emutls.py" "$gpu/PS5_PayloadSDK/libc/emutls.c" "$test_work/emutls.c"
clang -std=c11 -fsanitize=address -c "$test_work/cxa.c" -o "$test_work/cxa.o"
clang -std=c11 -D__PROSPERO__ -fsanitize=address -c "$test_work/emutls.c" -o "$test_work/emutls.o"
clang++ -std=c++20 -femulated-tls -fsanitize=address -pthread "$root/tests/native_tls.cpp" "$test_work/cxa.o" "$test_work/emutls.o" -o "$test_work/tls-test"
ASAN_OPTIONS=detect_leaks=0 "$test_work/tls-test"
clang -std=c11 -Wall -Wextra -Werror $(pkg-config --cflags sdl2) -I "$root/src" "$root/tests/native_controller.c" $(pkg-config --libs sdl2) -o "$test_work/controller-test"
SDL_VIDEODRIVER=dummy "$test_work/controller-test"
