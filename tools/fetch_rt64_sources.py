#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fetch the exact public RT64 inputs into a dedicated fresh build directory."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile
import urllib.request

if len(sys.argv) != 3:
    raise SystemExit('Usage: fetch_rt64_sources.py ABSOLUTE_RT64_WORK ABSOLUTE_GPU_WORK')
work, gpu = map(Path, sys.argv[1:])
if not work.is_absolute() or not gpu.is_absolute():
    raise SystemExit('Use absolute dedicated work paths')
root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'RT64_SOURCES.json').read_text())
(work / 'downloads').mkdir(parents=True, exist_ok=True)

def download(path, url, expected):
    if not path.exists():
        part = path.with_name(path.name + '.part')
        with urllib.request.urlopen(url, timeout=60) as response, part.open('wb') as output:
            shutil.copyfileobj(response, output)
        if hashlib.sha256(part.read_bytes()).hexdigest() != expected:
            raise SystemExit('Downloaded input checksum mismatch: ' + str(path))
        part.rename(path)
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise SystemExit('Input checksum mismatch: ' + str(path))

for row in manifest['sources']:
    archive = (root / 'vendor' if row.get('bundled') else work / 'downloads') / row['file']
    download(archive, row.get('url'), row['sha256'])
    target = work / row['path']
    stamp = target / '.dkrr-source-sha256'
    if stamp.exists():
        if stamp.read_text().strip() != row['sha256']:
            raise SystemExit('Existing source has another pin: ' + str(target))
        continue
    if target.exists() and any(target.iterdir()):
        raise SystemExit('Refusing to overwrite unstamped source: ' + str(target))
    target.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            if row['strip_components']:
                parts = member.name.split('/', 1)
                if len(parts) < 2 or not parts[1]:
                    continue
                member.name = parts[1]
            tar.extract(member, target, filter='data')
    stamp.write_text(row['sha256'] + '\n')

plume = work / 'rt64/src/contrib/plume'
marker = plume / '.dkrr-ps5-copy'
if not marker.exists():
    if plume.exists() and any(plume.iterdir()):
        raise SystemExit('Refusing to overwrite populated Plume tree')
    shutil.copytree(gpu / 'Plume', plume, dirs_exist_ok=True)
    marker.write_text('Validated GPU-work Plume; verify with apply_plume_patch.py\n')

for row in manifest['dxc']['files']:
    target = work / 'rt64/src/contrib/dxc' / row['path']
    target.parent.mkdir(parents=True, exist_ok=True)
    download(target, row['url'], row['sha256'])
    if row['path'].startswith('bin/'):
        target.chmod(0o755)
print('Pinned RT64 sources and host DXC verified')
