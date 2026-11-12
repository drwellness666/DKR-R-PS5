#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""List functions with large stack frames in a linked ELF and compare with a baseline.

A big local buffer is harmless on the main thread and can overflow a worker
thread's stack. The console's default thread stack size is measured by the
start-up self-test (logs/selftest.json, "default thread stack size").
Frames come from the prologue (`sub $N,%rsp`, or a stack-probe loop's total), so
the check needs no recompilation and works on the exact linked image.

  check_stack_frames.py <llvm-pie.elf> --baseline docs/STACK_BASELINE.json [--update]
Exit status 1 if a function above the limit is new or grew beyond its baseline.
"""
import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('elf', type=pathlib.Path)
parser.add_argument('--baseline', type=pathlib.Path, required=True)
parser.add_argument('--limit', type=int, default=16384, help='report frames of at least this many bytes')
parser.add_argument('--update', action='store_true', help='record the current list as the accepted baseline')
args = parser.parse_args()

objdump = shutil.which('llvm-objdump') or shutil.which('objdump')
if not objdump:
    sys.exit('Need llvm-objdump or objdump')
header = re.compile(r'^[0-9a-f]+ <(.+)>:$')
sub = re.compile(r'\bsub[ql]?\s+\$0x([0-9a-f]+),\s*%rsp')
frames = {}
process = subprocess.Popen([objdump, '-d', '--no-show-raw-insn', '-C', str(args.elf)], stdout=subprocess.PIPE, text=True, errors='replace')
name, seen = None, 0
for line in process.stdout:
    match = header.match(line)
    if match:
        name, seen = match.group(1), 0
        continue
    if name and seen < 40:  # the prologue is at the start of the function
        seen += 1
        found = sub.search(line)
        if found:
            size = int(found.group(1), 16)
            if size >= args.limit and size > frames.get(name, 0):
                frames[name] = size
if process.wait() != 0:
    sys.exit('objdump failed')
ordered = dict(sorted(frames.items(), key=lambda item: -item[1]))
for function, size in list(ordered.items())[:25]:
    print(f'{size:>9}  {function[:150]}')
print(f'{len(ordered)} function(s) with a frame of at least {args.limit} bytes')
if args.update:
    args.baseline.write_text(json.dumps({'limit': args.limit, 'frames': ordered}, indent=1) + '\n')
    print(f'Baseline written: {args.baseline}')
    sys.exit(0)
accepted = json.loads(args.baseline.read_text())['frames'] if args.baseline.exists() else {}
new = {f: s for f, s in ordered.items() if s > accepted.get(f, 0)}
for function, size in new.items():
    print(f'NEW/GROWN: {size} bytes (was {accepted.get(function, 0)}) {function[:150]}')
sys.exit(1 if new else 0)
