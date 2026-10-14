#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Renderer library and synthetic native title only; does not deploy or load a ROM.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${DKR_GPU_WORK:?Set DKR_GPU_WORK to the completed GPU build directory}"
: "${DKR_RT64_WORK:?Set DKR_RT64_WORK to a dedicated fresh renderer build directory}"
: "${DKR_SDL_WORK:?Set DKR_SDL_WORK to the completed lowmem SDL build directory}"
[[ $DKR_GPU_WORK == /* && $DKR_RT64_WORK == /* && $DKR_SDL_WORK == /* ]] || exit 2
cmake_bin=${CMAKE_BIN:-cmake}
python3 "$root/tools/fetch_rt64_sources.py" "$DKR_RT64_WORK" "$DKR_GPU_WORK"
python3 "$root/tools/apply_rt64_patch.py" "$DKR_RT64_WORK/rt64"
python3 "$root/tools/apply_plume_patch.py" "$DKR_RT64_WORK/rt64/src/contrib/plume"
mkdir -p "$DKR_RT64_WORK/host-tools"
clang++ -std=c++17 -O2 "$DKR_RT64_WORK/rt64/src/tools/file_to_c/file_to_c.cpp" \
    -o "$DKR_RT64_WORK/host-tools/file_to_c"
sdk="$DKR_GPU_WORK/PS5_Vulkan/.deps/native/ps5-payload-sdk"
"$cmake_bin" -S "$DKR_RT64_WORK/rt64" -B "$DKR_RT64_WORK/build" \
    -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" -DCMAKE_BUILD_TYPE=Release \
    -DRT64_STATIC=ON -DRT64_SDL_WINDOW_VULKAN=OFF -DPLUME_BUILD_EXAMPLES=OFF \
    -DDKR_PS5_FILE_TO_C="$DKR_RT64_WORK/host-tools/file_to_c" \
    -DDKR_PS5_SDL_SOURCE="$DKR_SDL_WORK/src/ps5-SDL" \
    -DDKR_PS5_SDL_ARCHIVE="$DKR_SDL_WORK/build/sdl/libSDL2.a"
"$cmake_bin" --build "$DKR_RT64_WORK/build" --target rt64 --parallel "${DKR_GPU_JOBS:-4}"
export DKR_RT64_WORK DKR_SDL_WORK
DKR_RT64_SMOKE=1 bash "$root/tools/build_native_gpu_title.sh" "$DKR_GPU_WORK"
