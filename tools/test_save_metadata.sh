#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
[[ $# == 1 && $1 == /* ]] || { echo 'Usage: test_save_metadata.sh ABSOLUTE_TEST_WORK' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
mkdir -p "$1"
clang -std=c11 -D_DEFAULT_SOURCE -O1 -Wall -Wextra -Werror -I "$root/src" -Dsocket=test_socket -Dsetsockopt=test_setsockopt -Dconnect=test_connect -Dsend=test_send -Drecv=test_recv -Dclose=test_close -c "$root/src/ps5_save_stat.c" -o "$1/save-stat.o"
clang -std=c11 -D_DEFAULT_SOURCE -O1 -Wall -Wextra -Werror -I "$root/src" "$root/tests/save_metadata_model.c" "$1/save-stat.o" -o "$1/save-stat-model"
"$1/save-stat-model"
