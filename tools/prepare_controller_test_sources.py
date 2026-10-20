#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare only pinned public inputs needed for the ROM-free controller tests."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import urllib.request
from compose_upstream_release import compose as compose_release

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('work', type=Path)
parser.add_argument('--cache', type=Path, action='append', default=[])
args = parser.parse_args()
if not args.work.is_absolute() or args.work.exists():
    raise SystemExit('Use a new absolute test input directory')
args.work.mkdir(parents=True)

def fetch(row):
    target = args.work / row['file']
    for cache in [root / 'vendor', *args.cache]:
        candidate = cache / row['file']
        if candidate.is_file() and hashlib.sha256(candidate.read_bytes()).hexdigest() == row['sha256']:
            shutil.copyfile(candidate, target)
            break
    else:
        with urllib.request.urlopen(row['url'], timeout=30) as response, target.open('wb') as out:
            total = 0
            while data := response.read(1024 * 1024):
                total += len(data)
                if total > 96 * 1024 * 1024:
                    raise SystemExit('Public test source archive exceeds the limit')
                out.write(data)
    if hashlib.sha256(target.read_bytes()).hexdigest() != row['sha256']:
        raise SystemExit('Pinned public test source checksum mismatch: ' + row['file'])
    return target

game_row = next(row for row in json.loads((root / 'GAME_SOURCES.json').read_text())['sources'] if row['name'] == 'DKR-R')
imgui_row = next(row for row in json.loads((root / 'RT64_SOURCES.json').read_text())['sources'] if row.get('name') == 'imgui')
manifest = json.loads((root / 'patches/dkrr-patch-manifest.json').read_text())
patch = root / 'patches/0005-dkrr-ps5-native-preview.patch'
if hashlib.sha256(patch.read_bytes()).hexdigest() != manifest['patch_sha256']:
    raise SystemExit('Authored controller patch checksum mismatch')
game = args.work / 'source/DKR-R'
wanted = set(manifest['files']) | {
    'runtime-recomp/src/game/revision_addresses.hpp',
    'runtime-recomp/src/game/rom_revision.hpp',
    'runtime-recomp/src/game/controller_snapshot.hpp',
    'runtime-recomp/src/game/motion_steering_policy.hpp',
    'runtime-recomp/src/game/runtime_enhancements.hpp',
    'runtime-recomp/src/game/modern_camera_policy.hpp',
    'runtime-recomp/src/game/presentation_policy.hpp',
    'runtime-recomp/src/game/generated/jumpman_font.c',
    'runtime-recomp/src/game/generated/jumpman_font.h',
    'runtime-recomp/src/game/generated/racing_banana_font.c',
    'runtime-recomp/src/game/generated/racing_banana_font.h',
    'runtime-recomp/src/game/generated/selawik_font.c',
    'runtime-recomp/src/game/generated/selawik_font.h',
}
game_config = json.loads((root / 'GAME_SOURCES.json').read_text())
with tarfile.open(fetch(game_row)) as archive:
    for member in archive:
        parts = PurePosixPath(member.name).parts
        if len(parts) < 2:
            continue
        member.name = '/'.join(parts[1:])
        archive.extract(member, game, filter='data')
if game_config.get('release_layers'):
    compose_release(game, game_config['release_layers']['sha256'])
for name in wanted:
    path = game / name
    if not path.is_file():
        raise SystemExit('Incomplete pinned controller input: ' + name)
    if name in manifest['files'] and hashlib.sha256(path.read_bytes()).hexdigest() != manifest['files'][name]['original_sha256']:
        raise SystemExit('Pristine effective controller test input mismatch: ' + name)
subprocess.run(['patch', '--batch', '--fuzz=0', '-p1', '-d', str(game), '-i', str(patch)], check=True)
for name, row in manifest['files'].items():
    if hashlib.sha256((game / name).read_bytes()).hexdigest() != row['patched_sha256']:
        raise SystemExit('Patched controller test input mismatch: ' + name)
imgui = args.work / 'source/rt64-game/src/contrib/imgui'
with tarfile.open(fetch(imgui_row)) as archive:
    for member in archive:
        parts = PurePosixPath(member.name).parts
        # Only regular top-level files are required, including ImGui's stb headers.
        if not member.isfile() or len(parts) != 2 or parts[1] in {'.', '..'}:
            continue
        out = imgui / parts[1]
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(archive.extractfile(member).read())
# The native picker exercises the real NFD ABI. ImGui's archive does not
# contain that independently pinned dependency's header.
nfd_row = next(row for row in json.loads((root / 'GAME_SOURCES.json').read_text())['sources']
               if row['path'].endswith('/nativefiledialog-extended'))
with tarfile.open(fetch(nfd_row)) as archive:
    headers = [member for member in archive if member.isfile()
               and '/'.join(PurePosixPath(member.name).parts[1:]) == 'src/include/nfd.h']
    if len(headers) != 1:
        raise SystemExit('Incomplete or duplicate pinned NFD test header')
    target = args.work / 'source' / nfd_row['path'] / 'src/include/nfd.h'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(archive.extractfile(headers[0]).read())
print('PASS pinned public controller/ImGui/NFD test inputs; no ROM or game payload used')
