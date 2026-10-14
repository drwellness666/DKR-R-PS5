#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds diagnostics only. Never presents a partial runtime as a game port.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${DKR_PS5_WORK:?Set DKR_PS5_WORK to an absolute, dedicated build directory}"
[[ $DKR_PS5_WORK == /* ]] || { echo 'DKR_PS5_WORK must be absolute' >&2; exit 2; }
cmake_bin=${CMAKE_BIN:-cmake}
for tool in "$cmake_bin" make curl unzip tar python3 sha256sum patch; do
    command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 2; }
done
mkdir -p "$DKR_PS5_WORK/src" "$DKR_PS5_WORK/downloads"
(cd "$root/vendor" && sha256sum -c SHA256SUMS)
for name in ps5-SDL libsamplerate; do
    if [[ ! -f $DKR_PS5_WORK/src/$name/CMakeLists.txt ]]; then
        mkdir -p "$DKR_PS5_WORK/src/$name"
        tar -xzf "$root/vendor/$name.tar.gz" -C "$DKR_PS5_WORK/src/$name"
    fi
done
python3 "$root/tools/apply_sdl_patch.py" "$DKR_PS5_WORK/src/ps5-SDL"
sdk_archive="$DKR_PS5_WORK/downloads/ps5-payload-sdk-v0.44.zip"
sdk_digest=4d06eab777bb135705b5a6816688c9132efb246093fe606ac488ab6cf65ca5b0
if [[ ! -f $sdk_archive ]]; then
    curl --fail --location --proto '=https' \
      https://github.com/ps5-payload-dev/sdk/releases/download/v0.44/ps5-payload-sdk.zip \
      -o "$sdk_archive.part"
    printf '%s  %s\n' "$sdk_digest" "$sdk_archive.part" | sha256sum -c -
    mv "$sdk_archive.part" "$sdk_archive"
fi
printf '%s  %s\n' "$sdk_digest" "$sdk_archive" | sha256sum -c -
sdk="$DKR_PS5_WORK/sdk/ps5-payload-sdk"
if [[ ! -x $sdk/bin/prospero-clang ]]; then
    mkdir -p "$DKR_PS5_WORK/sdk"
    unzip -q "$sdk_archive" -d "$DKR_PS5_WORK/sdk"
fi
"$cmake_bin" -S "$root" -B "$DKR_PS5_WORK/build" \
  -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS5_SDL_SOURCE="$DKR_PS5_WORK/src/ps5-SDL" \
  -DSAMPLERATE_SOURCE="$DKR_PS5_WORK/src/libsamplerate"
"$cmake_bin" --build "$DKR_PS5_WORK/build" --parallel "${DKR_PS5_JOBS:-4}"
printf '\nNative diagnostic ELF files (see README for hardware results):\n%s\n%s\n%s\n%s\n' \
  "$DKR_PS5_WORK/build/dkrr-ps5-probe.elf" \
  "$DKR_PS5_WORK/build/dkrr-ps5-cxx-probe.elf" \
  "$DKR_PS5_WORK/build/dkrr-ps5-memory-probe.elf" \
  "$DKR_PS5_WORK/build/dkrr-ps5-probe-lowmem.elf"
