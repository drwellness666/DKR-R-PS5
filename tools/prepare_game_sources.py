#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare pinned public game sources in a dedicated directory, without ROMs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('work', type=Path)
parser.add_argument('--cache', type=Path, action='append', default=[])
args = parser.parse_args()
args.cache.insert(0, root / 'vendor')
work = args.work
if not work.is_absolute():
    raise SystemExit('Use an absolute dedicated work directory')
source = work / 'source'
stamp = work / 'prepared-sources.json'
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def inventory():
    files, links = {}, {}
    for path in sorted(source.rglob('*')):
        if '.git' in path.relative_to(source).parts or '__pycache__' in path.relative_to(source).parts or path.suffix == '.pyc':
            continue
        if path.is_symlink():
            links[str(path.relative_to(source))] = str(path.readlink())
        elif path.is_file():
            files[str(path.relative_to(source))] = digest(path)
    inputs = ['GAME_SOURCES.json', 'RT64_SOURCES.json',
              'patches/0001-ultramodern-ps5-platform.patch',
              'patches/0003-plume-ps5-radv-display.patch',
              'patches/0004-rt64-ps5-build-platform.patch',
              'patches/0005-dkrr-ps5-native-preview.patch',
              'patches/dkrr-patch-manifest.json', 'src/plume_ps5_display.h', 'src/ps5_display_options.h']
    return {'files': files, 'links': links, 'inputs': {name:digest(root/name) for name in inputs}}

if stamp.exists():
    if inventory() != json.loads(stamp.read_text()):
        raise SystemExit('Prepared source tree changed; use a fresh directory')
    print('Prepared game sources match the complete saved inventory')
    raise SystemExit(0)
if source.exists() and any(source.iterdir()):
    raise SystemExit('Refusing to overwrite an unstamped populated source tree')
source.mkdir(parents=True, exist_ok=True)
(work / 'downloads').mkdir(parents=True, exist_ok=True)

def fetch(filename, url, expected, bundled=False):
    path = (root / 'vendor' if bundled else work / 'downloads') / filename
    if not path.exists() and not bundled:
        for cache in args.cache:
            candidate = cache / filename
            if candidate.is_file() and digest(candidate) == expected:
                shutil.copy2(candidate, path)
                break
    if not path.exists():
        if not url:
            raise SystemExit('Missing bundled source: ' + str(path))
        part = path.with_name(path.name + '.part')
        with urllib.request.urlopen(url, timeout=60) as response, part.open('wb') as output:
            shutil.copyfileobj(response, output)
        if digest(part) != expected:
            raise SystemExit('Downloaded source checksum mismatch: ' + filename)
        part.rename(path)
    if digest(path) != expected:
        raise SystemExit('Source checksum mismatch: ' + filename)
    return path

def unpack(row):
    path = fetch(row['file'], row.get('url'), row['sha256'], row.get('bundled', False))
    target = source / row['path']
    target.mkdir(parents=True, exist_ok=True)
    with tarfile.open(path) as archive:
        for member in archive.getmembers():
            if row.get('strip_components', 0):
                parts = member.name.split('/', 1)
                if len(parts) < 2 or not parts[1]:
                    continue
                member.name = parts[1]
            archive.extract(member, target, filter='data')

game = json.loads((root / 'GAME_SOURCES.json').read_text())
renderer = json.loads((root / 'RT64_SOURCES.json').read_text())
# Renderer archives precede nested Plume/NFD inputs from GAME_SOURCES.
for row in renderer['sources']:
    unpack(dict(row, path=row['path'].replace('rt64', 'rt64-game', 1)))
for row in game['sources']:
    unpack(row)
for row in renderer['dxc']['files']:
    target = source / 'rt64-game/src/contrib/dxc' / row['path']
    path = fetch('dxc-' + row['path'].replace('/', '-'), row['url'], row['sha256'])
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, target)
    if row['path'].startswith('bin/'):
        target.chmod(0o755)

dk = source / 'DKR-R'
targets = {'N64ModernRuntime': source / 'N64ModernRuntime',
           'N64Recomp': source / 'N64Recomp', 'sljit': source / 'N64Recomp/lib/sljit',
           'RT64': source / 'rt64-game', 'Plume': source / 'rt64-game/src/contrib/plume',
           'hlslpp': source / 'rt64-game/src/contrib/hlslpp'}
def apply(target, patch):
    cmd = ['git', '-C', str(target), 'apply', '--recount', '--whitespace=nowarn', '--ignore-space-change']
    subprocess.run(cmd + ['--check', str(patch)], check=True)
    subprocess.run(cmd + [str(patch)], check=True)

for dependency in json.loads((dk / 'patches/manifest.json').read_text())['dependencies']:
    if dependency['name'] not in targets:
        continue
    for row in dependency['patches']:
        patch = dk / row['path']
        if digest(patch) != row['sha256']:
            raise SystemExit('Authored dependency patch checksum mismatch')
        apply(targets[dependency['name']], patch)
for name, target in [('0001-ultramodern-ps5-platform.patch', source / 'N64ModernRuntime'),
                     ('0003-plume-ps5-radv-display.patch', targets['Plume']),
                     ('0004-rt64-ps5-build-platform.patch', targets['RT64'])]:
    cmd = ['patch', '--batch', '--fuzz=0', '-p1', '-d', str(target), '-i', str(root / 'patches' / name)]
    subprocess.run(cmd + ['--dry-run'], check=True)
    subprocess.run(cmd, check=True)
shutil.copy2(root / 'src/plume_ps5_display.h', targets['Plume'] / 'plume_ps5_display.h')
shutil.copy2(root / 'src/ps5_display_options.h', targets['Plume'] / 'ps5_display_options.h')
manifest = json.loads((root / 'patches/dkrr-patch-manifest.json').read_text())
patch = root / 'patches/0005-dkrr-ps5-native-preview.patch'
if digest(patch) != manifest['patch_sha256']:
    raise SystemExit('DKR PS5 patch checksum mismatch')
for name, hashes in manifest['files'].items():
    if digest(dk / name) != hashes['original_sha256']:
        raise SystemExit('DKR source differs from the reviewed original: ' + name)
cmd = ['patch', '--batch', '--fuzz=0', '-p1', '-d', str(dk), '-i', str(patch)]
subprocess.run(cmd + ['--dry-run'], check=True)
subprocess.run(cmd, check=True)
for name, hashes in manifest['files'].items():
    if digest(dk / name) != hashes['patched_sha256']:
        raise SystemExit('DKR patched source mismatch: ' + name)
# Both compiler and runtime use the same named recomp_context and SLJIT patch.
shutil.copytree(source / 'N64Recomp', source / 'N64ModernRuntime/N64Recomp', dirs_exist_ok=True)
for path, target in [('extern/n64-modern-runtime', '../../N64ModernRuntime'),
                     ('extern/rt64', '../../rt64-game')]:
    link = dk / path
    if link.exists():
        if any(link.iterdir()):
            raise SystemExit('Unexpected populated dependency placeholder: ' + str(link))
        link.rmdir()
    link.symlink_to(target, target_is_directory=True)
# FetchContent is prepopulated so CMake never silently chooses other sources.
xdelta = work / 'game-ps5/_deps/dkr-xdelta-3.2.0'
xdelta.mkdir(parents=True, exist_ok=True)
for name, sha in re.findall(r'dkr_xdelta_file\(([^ ]+) ([a-f0-9]{64})\)',
                            (dk / 'runtime-recomp/cmake/LegacyMods.cmake').read_text()):
    url = 'https://raw.githubusercontent.com/jmacd/xdelta/ff322e592383227b0d65ddfde7e0e5bbc504dc15/xdelta3/' + name
    shutil.copy2(fetch('xdelta-' + name, url, sha), xdelta / name)
stamp.write_text(json.dumps(inventory(), indent=2) + '\n')
print('Pinned public sources, authored patches and PS5 adapters prepared; no ROM used')
