#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
[[ $# == 1 && $1 == /* ]] || { echo 'Usage: build_save_metadata.sh ABSOLUTE_SDL_SDK' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
mkdir -p "$root/bin"
"$1/bin/prospero-clang" -std=c11 -O2 -Wall -Wextra -Werror -I "$root/src" "$root/src/ps5_save_metadata_server.c" -o "$root/bin/dkrr-save-metadata.elf"
echo 'Built read-only loopback metadata helper; start it before the native title on affected nullfs views.'
