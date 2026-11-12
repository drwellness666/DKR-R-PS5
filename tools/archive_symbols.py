#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Keep the unstripped ELF of a release under its build id for crash analysis.

The shipped eboot is stripped; without the matching llvm-pie.elf a crash
address cannot be resolved. Run after linking:
  archive_symbols.py <native-link/llvm-pie.elf> <build-id> <symbols-directory>
The directory is local only (it is large and never published).
"""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('elf', type=pathlib.Path)
parser.add_argument('build_id')
parser.add_argument('symbols', type=pathlib.Path)
args = parser.parse_args()
if not re.fullmatch(r'[a-z0-9-]{1,64}', args.build_id):
    parser.error('build id must match the journal grammar [a-z0-9-]')
if args.elf.read_bytes()[:4] != b'\x7fELF':
    parser.error('not an ELF')
destination = args.symbols / args.build_id
destination.mkdir(parents=True, exist_ok=True)
target = destination / 'llvm-pie.elf'
shutil.copyfile(args.elf, target)
# `llvm-nm` is also what the report tool uses; check the entry symbol exists now,
# not on the evening something has crashed.
names = subprocess.run([shutil.which('llvm-nm') or 'nm', '--defined-only', str(target)], capture_output=True, text=True, check=True).stdout
if not re.search(r'\s[Tt] _start$', names, re.M):
    raise SystemExit('The archived ELF has no _start symbol; crash reports could not compute the image base.')
(destination / 'ARCHIVE.json').write_text(json.dumps({
    'build_id': args.build_id,
    'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
    'size': target.stat().st_size,
}, indent=1) + '\n')
print(f'Archived {target} ({target.stat().st_size} bytes)')
