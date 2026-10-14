#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Entire matching decompilation/recompilation stays in a private local directory.
set -euo pipefail
: "${DKR_GAME_WORK:?Set absolute DKR_GAME_WORK}"
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
game=$DKR_GAME_WORK
[[ $game == /* ]] || exit 2
jobs=${DKR_GPU_JOBS:-4}
cmake_bin=${CMAKE_BIN:-cmake}
mkdir -p "$game/private" "$game/host-tools/binutils-build"
chmod 0700 "$game/private"
if [[ -n ${DKR_ROM:-} ]]; then
    python3 "$root/tools/prepare_game_roms.py" "$game/private/roms" --rom "$DKR_ROM"
else
    : "${DKR_ROM_V77:?Set DKR_ROM to one own dump, or set both DKR_ROM_V77 and DKR_ROM_V80}"
    : "${DKR_ROM_V80:?Set path to own US Rev A dump for the optional dual build}"
    python3 "$root/tools/prepare_game_roms.py" "$game/private/roms" "$DKR_ROM_V77" "$DKR_ROM_V80"
fi
mapfile -t revisions < <(python3 -c 'import json,sys;print(*json.load(open(sys.argv[1]))["revisions"],sep="\n")' "$game/private/roms/selected-revisions.json")
[[ ${#revisions[@]} -ge 1 ]] || exit 2
"$cmake_bin" -S "$game/source/N64Recomp" -B "$game/host-recompiler" -DCMAKE_BUILD_TYPE=Release
"$cmake_bin" --build "$game/host-recompiler" --target N64RecompCLI --parallel "$jobs"
if [[ ! -f $game/host-tools/binutils-build/Makefile ]]; then
    (cd "$game/host-tools/binutils-build" && "$game/source/binutils-2.46.0/configure" \
        --target=mips64-elf --disable-nls --disable-werror --disable-gdb --disable-gprof --disable-gold)
fi
make -C "$game/host-tools/binutils-build" -j"$jobs" all-binutils all-gas all-ld
if [[ ! -x $game/host-tools/decomp-venv/bin/python3 ]]; then
    python3 -m venv "$game/host-tools/decomp-venv"
fi
"$game/host-tools/decomp-venv/bin/python3" -m pip install -r "$root/tools/decomp-python.lock.txt"
python3 - "$game" <<'PY'
from pathlib import Path
import shutil,sys
game=Path(sys.argv[1])
import json
for revision in json.loads((game/'private/roms/selected-revisions.json').read_text())['revisions']:
    target=game/'private'/('v%d'%revision)
    if target.exists():
        raise SystemExit('Refusing an existing decomp tree; use a fresh game directory: '+str(target))
    shutil.copytree(game/'source/dkr-decomp',target)
    (target/'.venv').symlink_to(game/'host-tools/decomp-venv',target_is_directory=True)
    binaries=target/'tools/binutils'
    binaries.mkdir(exist_ok=True)
    (binaries/'.dkrr-local-toolchain').write_text('Local public GNU binutils; never run get-binutils.sh\n')
    for name,folder,binary in [('ar','binutils','ar'),('as','gas','as-new'),('ld','ld','ld-new'),
                               ('objcopy','binutils','objcopy'),('objdump','binutils','objdump'),('strip','binutils','strip-new')]:
        shutil.copy2(game/'host-tools/binutils-build'/folder/binary,binaries/('mips64-elf-'+name))
    (target/'baseroms').mkdir(exist_ok=True)
    shutil.copy2(game/'private/roms'/('dkr.us.v%d.z64'%revision),target/'baseroms'/('baserom.us.v%d.z64'%revision))
PY
for revision in "${revisions[@]}"; do
    make -C "$game/private/v$revision/tools" -j"$jobs" n64crc dkr_assets_tool
    make -C "$game/private/v$revision" -j"$jobs" VERSION="v$revision" COLOR=0 extract
    # Verification requires the rebuilt ROM to match the original known SHA1.
    make -C "$game/private/v$revision" -j"$jobs" VERSION="v$revision" COLOR=0
done
pipeline_args=(--characters --character-menu --european-languages-policy "$root/tools/compose_game_languages.py" --revisions "${revisions[@]}"
    --recompiler "$game/host-recompiler/N64Recomp" --output "$game/private/pipeline")
for revision in "${revisions[@]}"; do
    pipeline_args+=("--v$revision-build" "$game/private/v$revision/build")
done
python3 "$game/source/DKR-R/scripts/generate_legacy_menu_qualification.py" "${pipeline_args[@]}"
echo 'Private matching CPU payloads generated; no ROM upload or deployment'
