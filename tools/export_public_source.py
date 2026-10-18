#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Export a positive source allowlist; never publish or copy private work directories."""
import hashlib
import re
from pathlib import Path
import shutil
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit('Usage: export_public_source.py ABSOLUTE_NEW_EXPORT_DIRECTORY')
root = Path(__file__).resolve().parent.parent
output = Path(sys.argv[1])
if not output.is_absolute() or output.exists():
    raise SystemExit('Use a new absolute export directory')
output.mkdir(parents=True)
files = ['.gitattributes','LICENSE','CMakeLists.txt','build.sh','build_gpu.sh','build_rt64.sh','build_game.sh','build_integrated.sh','setup.py','Start-Windows.cmd','Start-Linux.sh','README.md','README.de.md','CHANGELOG.md','PORT_RELEASE.json','GAME_SOURCES.json','RT64_SOURCES.json','VULKAN_SOURCES.json','TITLE_ART_SOURCES.json','CONTRIBUTING.md','THIRD_PARTY.md','CREDITS.md']
for name in files:
    shutil.copy2(root / name, output / name)
# Publish player help and the necessary contributor/build references only.
# Working audits and private preview reports are deliberately not in this list.
public_docs = ('README.md', 'INSTALLER.md', 'UPDATES.md', 'CONTROLS.md',
               'DISPLAY.md', 'SAVES.md', 'KNOWN_ISSUES.md', 'DIAGNOSTICS.md',
               'FILES.md', 'LICENSES.md', 'ROADMAP.md', 'DEVELOPMENT.md',
               'BUILD.md', 'WINDOWS.md', 'ARCHITECTURE.md', 'GPU.md', 'RT64.md')
(output / 'docs').mkdir()
for name in public_docs:
    shutil.copy2(root / 'docs' / name, output / 'docs' / name)
(output / 'updates').mkdir()
shutil.copy2(root / 'updates/channel.txt', output / 'updates/channel.txt')
shutil.copy2(root / 'updates/native-channel.txt', output / 'updates/native-channel.txt')
for folder in ('src', 'tools', 'patches', 'tests', 'licenses', 'installer'):
    shutil.copytree(root / folder, output / folder,
                    ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
archives = ['N64ModernRuntime.tar.gz', 'N64Recomp.tar.gz', 'libsamplerate.tar.gz',
            'ps5-SDL.tar.gz', 'ps5-sdk-source.tar.gz', 'Plume.tar.gz',
            'PS5_Vulkan-build-support.tar.gz', 'RT64.tar.gz',
            'zlib-1.3.1.tar.gz', 'zlib-1.3.2.tar.gz', 'zlib_1.3.1-1_patch.zip']
(output / 'vendor').mkdir()
for name in archives:
    shutil.copy2(root / 'vendor' / name, output / 'vendor' / name)
icon = output / 'homebrew/DKR-R-Probe/sce_sys/icon0.png'
icon.parent.mkdir(parents=True)
shutil.copy2(root / 'homebrew/DKR-R-Probe/sce_sys/icon0.png', icon)
shutil.copytree(root / 'homebrew/DKR-R-Game', output / 'homebrew/DKR-R-Game',
                ignore=shutil.ignore_patterns('background-source.png', 'launch-background-source.png'))
shutil.copytree(root / '.github/ISSUE_TEMPLATE', output / '.github/ISSUE_TEMPLATE')
shutil.copy2(root / '.github/pull_request_template.md', output / '.github/pull_request_template.md')
(output / '.gitignore').write_text('/work/\n/private/\n/native/\n/bin/\n/evidence/\n__pycache__/\n*.pyc\n*.z64\n*.v64\n*.n64\n*.o2r\n*.mpk\n*.eep\n*.sra\n*.fla\n*.elf\n*.bin\n*.prx\n*.sprx\n*.a\n*.o\n*.pcm\n*.dkru\n*.wav\n*.pem\n*.key\n')
workflow = output / '.github/workflows/source-check.yml'
workflow.parent.mkdir(parents=True)
shutil.copy2(root / '.github/workflows/source-check.yml', workflow)
subprocess.run([sys.executable, str(output / 'tools/check_public_source.py'), str(output)], check=True)
for folder in (output / 'vendor', output):
    paths = sorted(p for p in folder.rglob('*') if p.is_file() and p.name != 'SHA256SUMS' and '__pycache__' not in p.parts)
    (folder / 'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + str(p.relative_to(folder)) + '\n' for p in paths))
print('Public source export created:', output, '; no GitHub publication performed')
