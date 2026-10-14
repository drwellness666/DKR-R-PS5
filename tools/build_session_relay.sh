#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
[[ $# == 2 && $1 == /* && $2 == /* && $2 == */private/* ]] || { echo 'Usage: build_session_relay.sh ABSOLUTE_SDK ABSOLUTE/private/NEW_RELAY.elf' >&2; exit 2; }
[[ ! -e $2 && ! -L $2 ]] || { echo 'Output already exists' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
"$1/bin/prospero-clang" -std=c11 -O2 -Wall -Wextra -Werror "$root/src/ps5_session_relay.c" -o "$2"
"$1/bin/prospero-strip" --strip-debug "$2"
sha256sum "$2"
echo 'Optional local relay built. No console connection or deployment.'
