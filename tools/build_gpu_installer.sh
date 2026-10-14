#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
[[ $# == 2 || $# == 3 ]] || { echo 'Usage: build_gpu_installer.sh SDK_ROOT GPU_ELF [VERSIONED_DESTINATION]' >&2; exit 2; }
sdk=$1
gpu=$2
destination=${3:-/data/homebrew/DKR-R-Vulkan}
[[ $destination =~ ^/data/homebrew/DKR-R-Vulkan(-v[0-9]+)?$ ]] || { echo 'Invalid diagnostic destination' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work="$root/work/vulkan/installer"
mkdir -p "$work"
python3 - "$gpu" "$root/homebrew/DKR-R-Probe/sce_sys/icon0.png" "$work/embedded.S" <<'PY'
import pathlib,sys
gpu,icon,output=map(pathlib.Path,sys.argv[1:])
if gpu.read_bytes()[:4] != b'\x7fELF': raise SystemExit('Not an ELF')
def quoted(path):
    value=str(path.resolve())
    if any(c in value for c in '\n\r"\\'): raise SystemExit('Unsupported path')
    return '"'+value+'"'
text='.section .rodata\n'
for name,path in [('elf',gpu),('icon',icon)]:
    text+='.balign 16\n.global dkrr_gpu_'+name+'_start\n.global dkrr_gpu_'+name+'_end\n'
    text+='dkrr_gpu_'+name+'_start:\n.incbin '+quoted(path)+'\ndkrr_gpu_'+name+'_end:\n'
output.write_text(text)
PY
"$sdk/bin/prospero-clang" -c "$work/embedded.S" -o "$work/embedded.o"
"$sdk/bin/prospero-clang" -std=c11 -O2 -Wall -Wextra -Werror "-DDKR_INSTALL_HOME=\"$destination\"" \
    "$root/src/install_plume_probe.c" "$work/embedded.o" -o "$work/dkrr-install-plume-probe.elf"
echo "Built $work/dkrr-install-plume-probe.elf"
