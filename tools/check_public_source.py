#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail closed on private game artifacts in a public source export, including tar members."""
from pathlib import Path, PurePosixPath
import sys
import tarfile
import zipfile

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
forbidden = {'.z64', '.v64', '.n64', '.o2r', '.mpk', '.eep', '.sra', '.fla',
             '.elf', '.prx', '.sprx', '.bin', '.o', '.a'}
magic = {bytes.fromhex(value) for value in ('80371240', '37804012', '40123780')}
failures = []
def check(name, first):
    parts = PurePosixPath(name).parts
    # These are the pinned SDK's BSD internal headers, not private game outputs.
    sdk_headers = {'vendor/ps5-sdk-source.tar.gz/include/freebsd/private/bsdstat/bsdstat.h',
                   'vendor/ps5-sdk-source.tar.gz/include/freebsd/private/ucl/ucl.h'}
    if '..' in parts or ('private' in parts and name not in sdk_headers) or 'RecompiledFuncs' in parts or 'generated-v77' in parts or 'generated-v80' in parts:
        failures.append(name + ': private/unsafe path')
    if PurePosixPath(name).suffix.lower() in forbidden:
        failures.append(name + ': excluded binary/game artifact')
    if first[:4] in magic:
        failures.append(name + ': Nintendo 64 ROM header')

count = 0
for path in root.rglob('*'):
    relative = path.relative_to(root)
    if '.git' in relative.parts or '__pycache__' in relative.parts:
        continue
    if path.is_symlink():
        failures.append(str(relative) + ': symlink in source export')
        continue
    if not path.is_file():
        continue
    with path.open('rb') as f:
        check(str(relative), f.read(4))
    count += 1
    if path.name.endswith(('.tar.gz', '.tar.xz')):
        with tarfile.open(path) as archive:
            for member in archive:
                if member.isfile():
                    with archive.extractfile(member) as f:
                        check(str(relative) + '/' + member.name, f.read(4))
                    count += 1
    elif path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            for member in archive.infolist():
                if not member.is_dir():
                    with archive.open(member) as f:
                        check(str(relative) + '/' + member.filename, f.read(4))
                    count += 1
if failures:
    raise SystemExit('\n'.join(failures))
print('PASS public source scan: %d files/archive members; no excluded artifact or ROM header' % count)
