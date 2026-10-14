#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# One dual-revision candidate, including the optional 120-Hz menu in one app.
set -euo pipefail
: "${DKR_ROM_V77:?Set your own US 1.0 dump}"
: "${DKR_ROM_V80:?Set your own US 1.1 dump}"
[[ -z ${DKR_ROM:-} ]] || { echo 'Use both explicit DKR_ROM_V77/V80 inputs for the integrated build, not DKR_ROM' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
export DKR_PS5_HIGH_REFRESH_BUILD=1
bash "$root/build_game.sh"
