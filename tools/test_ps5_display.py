#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ROM-free sanitizer tests of actual PS5 display code, with modeled native APIs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cache', type=Path, action='append', default=[])
args = parser.parse_args()
archives = {x['name']: x for x in json.loads((ROOT / 'VULKAN_SOURCES.json').read_text())['sources']}
with tempfile.TemporaryDirectory(prefix='dkrr-display-') as temporary:
    work = Path(temporary)
    # Headers only from the already hash-pinned public SDK / Vulkan sources.
    for name, prefix in [('Vulkan-Headers', 'include/'), ('PS5_PayloadSDK', 'platform/include/')]:
        spec = archives[name]
        archive = next((p / spec['file'] for p in args.cache if (p / spec['file']).is_file()), work / spec['file'])
        if not archive.exists():
            with urllib.request.urlopen(spec['url'], timeout=60) as source:
                archive.write_bytes(source.read(64 * 1024 * 1024 + 1))
        if hashlib.sha256(archive.read_bytes()).hexdigest() != spec['sha256']:
            raise SystemExit(f'Pinned source hash mismatch: {name}')
        with tarfile.open(archive) as tar:
            selected = [(m, m.name.split(prefix, 1)[1]) for m in tar.getmembers()
                        if m.isfile() and prefix in m.name and m.name.endswith('.h')]
            if not selected:
                raise SystemExit(f'Header prefix absent in pinned {archive}: {prefix}')
            for member, name in selected:
                path = work / 'include' / name
                if '..' in Path(name).parts or Path(name).is_absolute():
                    raise SystemExit('Unsafe header path')
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(tar.extractfile(member).read())
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
    for hfr in (0, 1):
        output = work / f'display-{hfr}'
        subprocess.run(['clang++', '-std=c++20', '-O1', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                        f'-DDKR_DISPLAY_DATA_ROOT="{work / str(hfr)}"', f'-DDKR_PS5_HIGH_REFRESH_BUILD={hfr}', '-DDKR_PS5_GAME_DISPLAY=1',
                        '-I' + str(work / 'include'), '-I' + str(ROOT / 'src'),
                        str(ROOT / 'src/ps5_display_options.cpp'), str(ROOT / 'src/ps5_display_videoout.cpp'),
                        str(ROOT / 'tests/ps5_display.cpp'), '-o', str(output)], check=True)
        subprocess.run([str(output)], env=env, check=True)
