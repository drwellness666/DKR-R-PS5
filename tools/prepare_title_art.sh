#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Build native console formats from the actual pinned DKR-R project graphics.
set -euo pipefail
[[ $# == 2 && $1 == /* && $2 == /* ]] || { echo 'Usage: prepare_title_art.sh ABSOLUTE_GPU_WORK ABSOLUTE_GAME_WORK' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
support="$1/PS5_Vulkan"
inputs="$2/private/title-art-inputs"
output="$2/private/title-art-paired"
args=("$2" "$inputs")
[[ -z ${DKR_TITLE_HEADER_CACHE:-} ]] || args+=(--header-cache "$DKR_TITLE_HEADER_CACHE")
python3 "$root/tools/prepare_upstream_title_art.py" "${args[@]}"
encoder=${DKR_BC7ENC:-$support/.deps/native/bc7enc_rdo/build/bc7enc}
if [[ ! -x $encoder ]]; then
    if [[ ${CMAKE_BIN:-} == /* ]]; then
        env PATH="$(dirname -- "$CMAKE_BIN"):$PATH" bash "$support/tools/setup-asset-dependencies.sh"
    else
        bash "$support/tools/setup-asset-dependencies.sh"
    fi
fi
conversion=$(mktemp -d "$2/private/title-art-conversion.XXXXXX")
trap 'rm -rf -- "$conversion"' EXIT
# Use the same verified upstream image for selection and launch.
bash "$support/tools/prepare-assets.sh" \
    --icon "$inputs/icon-master.png" \
    --background "$inputs/background-master.png" \
    --bc7enc "$encoder" --output-directory "$conversion"
mkdir -p "$output"
for image in icon0.png pic0.dds pic1.dds; do
    cp "$conversion/$image" "$output/$image"
done
python3 "$root/tools/validate_title_art.py" "$output"
