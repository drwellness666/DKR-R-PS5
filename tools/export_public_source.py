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
files = ['.gitattributes', 'LICENSE', 'CMakeLists.txt', 'build.sh', 'build_gpu.sh', 'build_rt64.sh', 'build_game.sh',
         'PORT_RELEASE.json', 'GAME_SOURCES.json', 'RT64_SOURCES.json', 'VULKAN_SOURCES.json', 'TITLE_ART_SOURCES.json', 'TITLE_PRESENTATION.md',
         'GAME_BUILD.md', 'PUBLICATION.md', 'LICENSE_REVIEW.md', 'BETA_TESTING.md', 'PRIVATE_BETA.md', 'UPDATE_GUIDE.md', 'DUALSENSE_CONCEPT.md', 'DISPLAY_FEATURES.md', 'VERIFICATION.md', 'WORK_STATUS.md', 'CONSOLE_FEATURES.md', 'RENDER_ISSUES.md', 'PAL_PORT.md', 'CONTRIBUTING.md', 'PUBLIC_README.md', 'GPU_BUILD.md', 'RT64_BUILD.md', 'THIRD_PARTY.md', 'CHECKS.json']
for name in files:
    shutil.copy2(root / name, output / name)
shutil.copy2(root / 'PUBLIC_README.md', output / 'README.md')
for folder in ('src', 'tools', 'patches', 'tests', 'licenses'):
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
for name in ('GPU_BUILD.md', 'RT64_BUILD.md'):
    path = output / name
    path.write_text(re.sub(r'\b(?:[0-9]{1,3}\.){3}[0-9]{1,3}\b', 'YOUR_PS5_LAN_IP', path.read_text()))
(output / '.gitignore').write_text('/work/\n/private/\n/native/\n/bin/\n/evidence/\n__pycache__/\n*.pyc\n*.z64\n*.v64\n*.n64\n*.o2r\n*.mpk\n*.eep\n*.sra\n*.fla\n*.elf\n*.bin\n*.prx\n*.sprx\n*.a\n*.o\n')
workflow = output / '.github/workflows/source-check.yml'
workflow.parent.mkdir(parents=True)
workflow.write_text('''name: ROM-free source checks
on: [push, pull_request]
permissions:
  contents: read
jobs:
  source:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683 # v4.2.2
      - run: python3 tools/check_public_source.py .
      - run: sha256sum --quiet -c SHA256SUMS
      - run: sudo apt-get update && sudo apt-get install --no-install-recommends -y libsdl2-dev
      - run: python3 tools/test_controller_presets.py
      - run: python3 tools/test_ps5_display.py
      - run: python3 tools/test_rom_checker.py
      - run: python3 tools/test_ps5_realpath.py
      - run: python3 tools/test_ps5_filesystem.py
      - run: python3 tools/test_ps5_getcwd.py
      - run: python3 tools/test_ps5_rdram.py
      - run: bash tools/test_save_metadata.sh "$PWD/work/save-metadata-tests"
      - run: python3 tools/test_upstream_title_art.py
      - run: python3 tools/test_title_metadata_audit.py
      - run: ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_title_poster_retirement.py
      - run: python3 tools/test_performance_summary.py
      - run: python3 tools/test_revision_selection.py
      - run: ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_game_language_bridge.py
      - run: ASAN_OPTIONS=detect_leaks=0 python3 tools/test_native_game_bundle_update.py
      - run: ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_preview_update.py
      - run: python3 tools/test_preview_receipt.py
      - run: python3 tools/test_source_release_update.py
      - run: clang++ -std=c++20 -O1 -fsanitize=address,undefined -Isrc tests/neutral_audio.cpp -o /tmp/neutral-audio && ASAN_OPTIONS=detect_leaks=0 /tmp/neutral-audio
      - run: clang -std=c11 -Wall -Wextra -Werror -Isrc tests/shader_cache.c -o /tmp/shader-cache && /tmp/shader-cache
      - run: clang -std=c11 -Wall -Wextra -Werror -Isrc tests/render_diagnostics.c -o /tmp/render-diagnostics && /tmp/render-diagnostics
      - run: bash -n build.sh build_gpu.sh build_rt64.sh build_game.sh tools/*.sh
''')
subprocess.run([sys.executable, str(output / 'tools/check_public_source.py'), str(output)], check=True)
for folder in (output / 'vendor', output):
    paths = sorted(p for p in folder.rglob('*') if p.is_file() and p.name != 'SHA256SUMS' and '__pycache__' not in p.parts)
    (folder / 'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + str(p.relative_to(folder)) + '\n' for p in paths))
print('Public source draft created:', output, '; no GitHub publication performed')
