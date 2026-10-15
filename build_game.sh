#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Private native game candidate, not a claim of hardware gameplay acceptance.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${DKR_GPU_WORK:?Set completed absolute GPU work directory}"
: "${DKR_RT64_WORK:?Set completed absolute RT64 work directory}"
: "${DKR_SDL_WORK:?Set completed absolute SDL work directory}"
: "${DKR_GAME_WORK:?Set dedicated absolute game work directory}"
for path in "$DKR_GPU_WORK" "$DKR_RT64_WORK" "$DKR_SDL_WORK" "$DKR_GAME_WORK"; do
    [[ $path == /* ]] || { echo 'Work paths must be absolute' >&2; exit 2; }
done
args=("$DKR_GAME_WORK")
if [[ -n ${DKR_GAME_CACHE:-} ]]; then args+=(--cache "$DKR_GAME_CACHE"); fi
args+=(--cache "$DKR_RT64_WORK/downloads")
args+=(--cache "$DKR_GPU_WORK/downloads")
python3 "$root/tools/prepare_game_sources.py" "${args[@]}"
# Bind explicit caller inputs to the revision selection even when reusing a pipeline.
if [[ -n ${DKR_ROM:-} ]]; then
    python3 "$root/tools/prepare_game_roms.py" "$DKR_GAME_WORK/private/roms" --rom "$DKR_ROM"
elif [[ -n ${DKR_ROM_V77:-} || -n ${DKR_ROM_V80:-} ]]; then
    : "${DKR_ROM_V77:?Set both paths for the optional dual build}"
    : "${DKR_ROM_V80:?Set both paths for the optional dual build}"
    python3 "$root/tools/prepare_game_roms.py" "$DKR_GAME_WORK/private/roms" "$DKR_ROM_V77" "$DKR_ROM_V80"
fi
if [[ -z ${DKR_PIPELINE:-} ]]; then
    bash "$root/tools/build_game_payloads.sh"
    DKR_PIPELINE="$DKR_GAME_WORK/private/pipeline"
fi
[[ $DKR_PIPELINE == /* ]] || { echo 'Use an absolute verified authored pipeline' >&2; exit 2; }
# Select the explicitly supplied ROM(s); directory leftovers never add a revision.
if [[ -f $DKR_GAME_WORK/private/roms/selected-revisions.json ]]; then
    selection="$DKR_GAME_WORK/private/roms/selected-revisions.json"
else
    echo 'Missing ROM selection: validate the owned dump before using DKR_PIPELINE' >&2
    exit 2
fi
mapfile -t revisions < <(python3 -c 'import json,sys;print(*json.load(open(sys.argv[1]))["revisions"],sep="\n")' "$selection")
[[ ${#revisions[@]} -ge 1 ]] || exit 2
revision_list=""
for revision in "${revisions[@]}"; do
    [[ $revision == 77 || $revision == 80 ]] || exit 2
    [[ -d $DKR_PIPELINE/generated-v$revision ]] || { echo "Missing owned generated revision $revision" >&2; exit 2; }
    revision_list+="${revision_list:+;}$revision"
done
cmake_bin=${CMAKE_BIN:-cmake}
line_tables=${DKR_PS5_LINE_TABLES:-0}
[[ $line_tables == 0 || $line_tables == 1 ]] || { echo 'DKR_PS5_LINE_TABLES must be 0 or 1' >&2; exit 2; }
release_flags='-O3 -DNDEBUG'
[[ $line_tables != 1 ]] || release_flags+=' -gline-tables-only'
source="$DKR_GAME_WORK/source"
sdk="$DKR_GPU_WORK/PS5_Vulkan/.deps/native/ps5-payload-sdk"
mkdir -p "$DKR_GAME_WORK/private"
chmod 0700 "$DKR_GAME_WORK/private"
"$cmake_bin" -S "$source/DKR-R/runtime-recomp" -B "$DKR_GAME_WORK/game-ps5" \
    -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS_RELEASE="$release_flags" -DCMAKE_CXX_FLAGS_RELEASE="$release_flags" \
    -DDKR_PS5_REVISIONS="$revision_list" \
    -DBUILD_TESTING=OFF -DDKR_RUNTIME_BUILD_RT64=ON -DDKR_RUNTIME_BUILD_SDL3_INPUT_HOST=OFF \
    -DDKR_NETPLAY_WEBRTC=OFF -DDKR_ENHANCED_WATER=OFF \
    -DDKR_GENERATED_SOURCE_V77="$DKR_PIPELINE/generated-v77" \
    -DDKR_GENERATED_SOURCE_V80="$DKR_PIPELINE/generated-v80" \
    -DDKR_GENERATED_RSP_SOURCE="$source/DKR-R/runtime-recomp/RecompiledRSP" \
    -DFETCHCONTENT_SOURCE_DIR_DKR_MODS_XZ="$source/xz" \
    -DDKR_PS5_SDL_SOURCE="$DKR_SDL_WORK/src/ps5-SDL" \
    -DDKR_PS5_SDL_ARCHIVE="$DKR_SDL_WORK/build/sdl/libSDL2.a" \
    -DDKR_PS5_FILE_TO_C="$DKR_RT64_WORK/host-tools/file_to_c" \
    -DDKR_PS5_ADAPTERS_ROOT="$root/src"
"$cmake_bin" --build "$DKR_GAME_WORK/game-ps5" --target DKRPortGame --parallel "${DKR_GPU_JOBS:-4}"
bash "$root/tools/link_native_game.sh" "$DKR_GPU_WORK" "$DKR_GAME_WORK"
echo 'Private native candidate built for the selected owned revision(s). Candidate-specific console acceptance remains required.'
