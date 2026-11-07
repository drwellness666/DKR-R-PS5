#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
[[ $(uname -m) == x86_64 && $(uname -s) == Linux ]] || { echo 'This build environment needs x86-64 Linux or Windows with WSL2.' >&2; exit 2; }
command -v python3 >/dev/null || { echo 'Install Python 3.12+ from your Linux distribution, then start again.' >&2; exit 2; }
if [[ ${1:-} == --stage-windows ]]; then
    # Copy only checksum-listed public source. ROMs and prior work never cross.
    root=$(python3 "$root/tools/stage_windows_source.py" "$root")
fi
if ! python3 "$root/tools/host_requirements.py" >/dev/null; then
    echo 'The build tools are not complete. The following distribution packages are needed:'
    mapfile -t install_argv < <(python3 - "$root" <<'PY'
import sys
sys.path.insert(0,sys.argv[1]+'/tools')
from host_requirements import package_plan
print(*package_plan(),sep='\n')
PY
)
    [[ ${#install_argv[@]} -gt 0 && -n ${install_argv[0]} ]] || { python3 "$root/tools/host_requirements.py"; exit 2; }
    printf '%q ' sudo "${install_argv[@]}"; printf '\n'
    read -r -p 'Install these build tools now? [y/N] ' choice
    [[ $choice == y || $choice == Y ]] || { echo 'No packages changed. Start again when ready.'; exit 0; }
    if [[ ${install_argv[0]} == apt-get ]]; then sudo apt-get update; fi
    sudo "${install_argv[@]}"
    python3 "$root/tools/host_requirements.py" || { echo 'Prerequisites remain incomplete. Read the missing item above.'; exit 2; }
fi
cd -- "$root"
exec python3 setup.py
