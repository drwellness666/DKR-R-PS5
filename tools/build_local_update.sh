#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit opt-in local rebuild followed by an exact-own-predecessor update pair.
set -euo pipefail
[[ $# == 3 && $1 == /* && $2 == /* && $3 =~ ^[0-9a-f]{64}$ ]] || {
 echo 'Usage: build_local_update.sh ABSOLUTE_PREVIOUS_APP ABSOLUTE_NEW_PRIVATE_BUILD_ROOT EXPECTED_PREVIOUS_MANIFEST_SHA256' >&2; exit 2;
}
previous=$1
build_root=$2
expected=$3
[[ $build_root == */private/* && ! -e $build_root ]] || { echo 'Use a new dedicated private build directory' >&2; exit 2; }
[[ -n ${DKR_ROM:-} || -n ${DKR_ROM_V77:-} || -n ${DKR_ROM_V80:-} ]] || { echo 'Supply your own supported US ROM paths through the documented environment' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
python3 "$root/tools/check_public_source.py" "$root"
# No old build tree is overwritten and no ROM/profile on the console is touched.
mkdir -p "$build_root"
export DKR_PS5_WORK="$build_root/lowmem"
export DKR_SDL_WORK="$DKR_PS5_WORK"
export DKR_GPU_WORK="$build_root/vulkan"
export DKR_RT64_WORK="$build_root/rt64"
export DKR_GAME_WORK="$build_root/game"
bash "$root/build.sh"
bash "$root/build_gpu.sh"
bash "$root/build_rt64.sh"
bash "$root/build_game.sh"
python3 "$root/tools/build_preview_update_set.py" "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GPU_WORK" \
 "$previous" "$DKR_GAME_WORK/private/native/PPSA94678" "$build_root/private/update-pair" \
 --expected-previous-manifest-sha256 "$expected"
python3 "$root/tools/run_preview_update.py" "$build_root/private/update-pair"
echo "Prepared private update and rollback: $build_root/private/update-pair"
echo 'No console operation. Keep DKR-R closed before an explicitly requested send.'
