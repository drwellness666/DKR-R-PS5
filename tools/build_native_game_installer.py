#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a private collision-refusing installer; owned ROMs require an explicit local option."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

with_roms = len(sys.argv) == 4 and sys.argv[3] == '--with-owned-roms'
preferences_only = len(sys.argv) == 4 and sys.argv[3] == '--preferences-only'
diagnostics_only = len(sys.argv) == 4 and sys.argv[3] == '--diagnostics-only'
support_only = preferences_only or diagnostics_only
if len(sys.argv) != 3 and not support_only and not with_roms:
    raise SystemExit('Usage: build_native_game_installer.py ABSOLUTE_SDL_SDK ABSOLUTE_GAME_WORK [--with-owned-roms|--preferences-only|--diagnostics-only]')
sdk, game = map(Path, sys.argv[1:3])
if not sdk.is_absolute() or not game.is_absolute():
    raise SystemExit('Use absolute work paths')
root = Path(__file__).resolve().parent.parent
app = game / 'private/native/PPSA94678'
work = game / ('private/combined-installer' if with_roms else 'private/diagnostics-installer' if diagnostics_only else 'private/preference-installer' if preferences_only else 'private/installer')
work.mkdir(parents=True, exist_ok=True)
if json.loads((app / 'sce_sys/param.json').read_text())['titleId'] != 'PPSA94678':
    raise SystemExit('Unexpected native title identity')
allowed = {'eboot.bin', 'sce_module/libc.prx', 'sce_sys/param.json', 'sce_sys/icon0.png',
           'sce_sys/pic0.dds', 'sce_sys/pic1.dds'}
files = []
for path in sorted(app.rglob('*')):
    if path.is_symlink():
        raise SystemExit('Symlinks forbidden in native title inputs')
    if not path.is_file():
        continue
    name = str(path.relative_to(app))
    owned_audio = name in {f'assets/controller-audio/us-v{revision}/{i}.pcm' for revision in (77,80) for i in range(9)}
    if name not in allowed and not owned_audio and not (name.startswith('assets/') and path.suffix.lower() in {'.png', '.bmp', '.json', '.txt', '.hlsl', '.md'}):
        raise SystemExit('Unapproved installer input: ' + name)
    if any(c in str(path) for c in '\n\r"\\'):
        raise SystemExit('Unsupported file path')
    files.append(name)
if support_only:
    # Keep the existing app identity as a collision guard. Embed no executable,
    # graphics, ROM or save merely to provision a small writeable choice folder.
    files = ['sce_sys/param.json']
sources = {name: app / name for name in files}
destinations = {name: '/data/homebrew/PPSA94678/' + name for name in files}
if with_roms:
    selected = json.loads((game / 'private/roms/selected-revisions.json').read_text())['revisions']
    if sorted(selected) != [77, 80]:
        raise SystemExit('The normal installer requires both owned US 1.0 and US 1.1 ROMs')
    for revision, expected in ((77,'0cb115d8716dbbc2922fda38e533b9fe63bb9670'), (80,'6d96743d46f8c0cd0edb0ec5600b003c89b93755')):
        rom = game / 'private/roms' / f'dkr.us.v{revision}.z64'
        if rom.is_symlink() or rom.stat().st_size != 12 * 1024 * 1024 or hashlib.sha1(rom.read_bytes()).hexdigest() != expected:
            raise SystemExit('Canonical owned ROM failed verification')
        if any(c in str(rom) for c in '\n\r"\\'):
            raise SystemExit('Unsupported working directory path')
        key = f'owned-rom-{revision}'
        files.append(key); sources[key] = rom; destinations[key] = f'/data/dkrr/roms/dkr.us.v{revision}.z64'
files.sort(key=lambda name: (name in ('sce_sys/param.json','eboot.bin'), name == 'eboot.bin', name))
assembly = '.section .rodata\n'
header = 'struct native_file { const char *path; const unsigned char *start,*end; unsigned mode; };\n'
for i, name in enumerate(files):
    assembly += f'.balign 16\n.global native_file_{i}_start\n.global native_file_{i}_end\nnative_file_{i}_start:\n.incbin "{sources[name]}"\nnative_file_{i}_end:\n'
    header += f'extern const unsigned char native_file_{i}_start[],native_file_{i}_end[];\n'
directories = {'/data/homebrew', '/data/homebrew/PPSA94678', '/data/dkrr', '/data/dkrr/roms','/data/dkrr/renderer','/data/dkrr/diagnostics'}
directories.update('/data/dkrr/profile-us-v' + revision for revision in ('77','80'))
directories.add('/data/dkrr/preferences')
if preferences_only:
    directories = {'/data', '/data/dkrr', '/data/dkrr/preferences'}
if diagnostics_only:
    directories = {'/data', '/data/dkrr', '/data/dkrr/diagnostics'}
for name in files:
    path = Path(destinations[name])
    if not support_only:
        directories.update(str(parent) for parent in path.parents if str(parent).startswith('/data/homebrew/PPSA94678'))
header += 'static const char *native_dirs[]={' + ','.join(json.dumps(s) for s in sorted(directories, key=lambda s:(s.count('/'),s))) + '};\n'
header += 'static const struct native_file native_files[]={\n'
for i, name in enumerate(files):
    mode = '0755' if name in ('eboot.bin', 'sce_module/libc.prx') else '0644'
    header += f'{{"{destinations[name]}",native_file_{i}_start,native_file_{i}_end,{mode}}},\n'
header += '};\n'
(work / 'embedded.S').write_text(assembly)
(work / 'native_game_files.h').write_text(header)
cc = str(sdk / 'bin/prospero-clang')
subprocess.run([cc, '-c', str(work / 'embedded.S'), '-o', str(work / 'embedded.o')], check=True)
output = game / ('private/dkrr-install.elf' if with_roms else 'private/dkrr-install-diagnostics.elf' if diagnostics_only else 'private/dkrr-install-preferences.elf' if preferences_only else 'private/dkrr-install-game-1.0.6-preview.elf')
installer_source = root / 'src/install_native_game.c'
if with_roms:
    installer_source = work / 'install_all.c'
    installer_source.write_text((root / 'src/install_native_game.c').read_text().replace(
        'DKR-R 1.0.6 native preview installer (private build; no ROM included)',
        'PRIVATE DKR-R INSTALLER: game and both owned US ROMs; never share this ELF').replace(
        'NATIVE INSTALL PASS. Files staged only; mounting and gameplay untested.',
        'DKR-R INSTALL PASS. Both owned revisions installed; launch from the PS5 icon.'))
subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I', str(work),
                str(installer_source), str(work / 'embedded.o'), '-o', str(output)], check=True)
# Keep loadable segments/imports intact, removing only unused host debug data.
subprocess.run([str(sdk / 'bin/prospero-strip'), '--strip-debug', str(output)], check=True)
output.chmod(0o600 if with_roms else 0o755)
print('Built private installer:', output, '; files:', len(files))
print('CONTAINS BOTH OWNED ROMS. Never upload or share this generated ELF.' if with_roms else 'No ROM or save input')
if preferences_only:
    print('Only the dedicated own preference directory and byte-matched title identity; no app code or graphics embedded.')
if diagnostics_only:
    print('Only the dedicated own journal directory and byte-matched title identity; no app code, graphics, ROM or save embedded. Existing directories retain their permissions.')
