#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the reviewed RT64 platform patch to exact original sources."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if len(sys.argv) != 2:
    raise SystemExit('Usage: apply_rt64_patch.py EXTRACTED_RT64_SOURCE')
root = Path(__file__).resolve().parent.parent
source = Path(sys.argv[1]).resolve()
patch = root / 'patches/0004-rt64-ps5-build-platform.patch'
manifest = json.loads((root / 'patches/rt64-patch-manifest.json').read_text())
if digest(patch) != manifest['patch_sha256']:
    raise SystemExit('RT64 patch checksum mismatch')
files = manifest['files']
current = {name: digest(source / name) for name in files}
if all(current[name] == hashes['patched_sha256'] for name, hashes in files.items()):
    print('RT64 PS5 patch already applied; hashes match')
else:
    if not all(current[name] == hashes['original_sha256'] for name, hashes in files.items()):
        raise SystemExit('Refusing an unreviewed or edited RT64 source tree')
    command = ['patch', '--batch', '--forward', '--fuzz=0', '-p1', '-d', str(source), '-i', str(patch)]
    subprocess.run(command + ['--dry-run'], check=True)
    subprocess.run(command, check=True)
    if not all(digest(source / name) == hashes['patched_sha256'] for name, hashes in files.items()):
        raise SystemExit('Patched RT64 checksum mismatch')
