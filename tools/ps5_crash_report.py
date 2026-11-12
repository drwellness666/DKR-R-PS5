#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Turn a DKR-R PS5 diagnostic journal into a symbolised crash report.

The journal (/data/dkrr/diagnostics/run-*.log) is written by the title itself:
  DKRR_SESSION  build id, pid and the runtime address of the entry point
  DKRR_STEP     timestamped step markers of the subsystems
  DKRR_CRASH    signal, fault address, RIP, RSP, stage
  DKRR_STACK    stack words that point into the image
The image base is  entry - offset(entry symbol)  where the offset comes from the
unstripped ELF archived for that build id (tools/archive_symbols.py). Nothing
here contacts a console.
"""
import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys


def tool(*names):
    """First available binary (LLVM or binutils spelling, with or without a version suffix)."""
    for name in names:
        if shutil.which(name):
            return name
    raise SystemExit('Need one of: ' + ', '.join(names))


def parse_journal(text):
    """Return the facts of one journal; unknown lines are kept as 'other'."""
    out = {'session': None, 'crash': None, 'stack': [], 'steps': [], 'ended': False, 'nullcall': None, 'other': []}
    for line in text.splitlines():
        fields = dict(re.findall(r'(\w[\w-]*)=(\S+)', line))
        if line.startswith('DKRR_SESSION '):
            out['session'] = fields
        elif line.startswith('DKRR_CRASH '):
            out['crash'] = {k: (int(v, 16) if v.startswith('0x') else v) for k, v in fields.items()}
        elif line.startswith('DKRR_STACK '):
            words = fields.get('words', '')
            out['stack'] = [int(w, 16) for w in words.split(',') if w.startswith('0x')]
            out['stack_rsp'] = int(fields['rsp'], 16) if 'rsp' in fields else None
        elif line.startswith('DKRR_NULLCALL '):
            out['nullcall'] = int(fields['return'], 16)
        elif line.startswith('DKRR_STEP '):
            m = re.match(r'DKRR_STEP t=(\d+) sub=(\S+) ?(.*)$', line)
            if m:
                out['steps'].append((int(m.group(1)), m.group(2), m.group(3)))
        elif line.startswith('DKRR_SESSION_END'):
            out['ended'] = True
        elif line.strip():
            out['other'].append(line)
    return out


def symbol_offset(elf, name):
    """Offset of a defined symbol in the unstripped ELF (llvm-nm)."""
    result = subprocess.run([tool('llvm-nm', 'nm'), '--defined-only', str(elf)], capture_output=True, text=True, check=True)
    for row in result.stdout.splitlines():
        parts = row.split()
        if len(parts) >= 3 and parts[2] == name:
            return int(parts[0], 16)
    raise SystemExit(f'Symbol {name} not found in {elf}')


def call_target(elf, offset):
    """Name the function the call that returns to `offset` was aimed at (llvm-objdump)."""
    objdump = tool('llvm-objdump', 'objdump')
    out = subprocess.run([objdump, '-d', '--no-show-raw-insn', f'--start-address={max(offset - 16, 0):#x}',
                          f'--stop-address={offset:#x}', str(elf)], capture_output=True, text=True).stdout
    for row in reversed(out.splitlines()):
        if re.search(r'\bcall', row):
            names = re.findall(r'<([^>+]+)(?:\+0x[0-9a-f]+)?>', row)
            return names[-1].replace('@plt', '') if names else None
    return None


def symbolise(elf, offsets):
    """Resolve image offsets to 'function (file:line)' with inlined frames."""
    if not offsets:
        return {}
    # With -i one address yields several function/location pairs and no
    # separator is printed, so resolve each address on its own.
    addr2line = tool('llvm-addr2line', 'addr2line')
    resolved = {}
    size = pathlib.Path(elf).stat().st_size
    for offset in offsets:
        if offset < 0 or offset > size:
            # Not inside this image: another module, the kernel or a wrong base.
            resolved[offset] = ['outside the image']
            continue
        one = subprocess.run([addr2line, '-f', '-C', '-i', '-e', str(elf), hex(offset)],
                             capture_output=True, text=True, check=True).stdout.splitlines()
        frames = [f'{one[i]} ({one[i + 1]})' for i in range(0, len(one) - 1, 2)]
        resolved[offset] = frames if frames and not all(f.startswith('?? ') for f in frames) else ['unresolved']
    return resolved


def report(text, symbols=None, entry_symbol='_start'):
    facts = parse_journal(text)
    lines = []
    session = facts['session'] or {}
    lines.append(f"build   : {session.get('build', 'unknown')}  pid={session.get('pid', '?')}")
    crash = facts['crash']
    if not facts['steps'] and not crash:
        lines.append('journal : no steps and no crash record')
    lines.append(f"ended   : {'normally (DKRR_SESSION_END)' if facts['ended'] else 'NO session end (crash, kill or power loss)'}")
    base = None
    elf = None
    if symbols and session.get('entry') and session.get('build'):
        directory = pathlib.Path(symbols) / session['build']
        elf = directory / 'llvm-pie.elf'
        if elf.is_file():
            base = int(session['entry'], 16) - symbol_offset(elf, entry_symbol)
            lines.append(f'image   : base=0x{base:x} (entry {session["entry"]} - {entry_symbol})')
        else:
            lines.append(f'symbols : none archived for this build at {directory}')
    if crash:
        lines.append(f"crash   : signal={crash.get('signal', 0)} code={crash.get('code', 0)} "
                     f"fault-address=0x{crash.get('address', 0):x} stage={crash.get('stage', 0)}")
        rip = crash.get('rip', 0)
        lines.append(f'rip     : 0x{rip:x}')
        if base is not None and elf is not None:
            for frame in symbolise(elf, [rip - base]).get(rip - base, []):
                lines.append(f'          at {frame}')
        if rip == 0 and facts['nullcall'] is not None:
            lines.append('NULL CALL: the process jumped to address 0 right after a call. That is how a function the platform does')
            lines.append('          not export fails (it stays bound to 0). Return address = the call site:')
            if base is not None and elf is not None:
                where = facts['nullcall'] - base
                for frame in symbolise(elf, [where]).get(where, []):
                    lines.append(f'          called from {frame}')
                target = call_target(elf, where) if 0 <= where <= elf.stat().st_size else None
                if target:
                    lines.append(f'          UNRESOLVED IMPORT (by the call instruction): {target}')
                    lines.append('          Define it in src/ps5_missing_libc.c (or avoid it) and add it to docs/IMPORTS_BASELINE.json.')
            else:
                lines.append(f"          return address 0x{facts['nullcall']:x} (no symbols to resolve it)")
        if facts['stack'] and base is not None and elf is not None:
            lines.append('stack   : return-address candidates, nearest the crash first (not a verified call chain)')
            offsets = [w - base for w in facts['stack']]
            names = symbolise(elf, offsets)
            for word, offset in zip(facts['stack'], offsets):
                lines.append(f'          0x{word:x}  {" <- ".join(names[offset])}')
        elif facts['stack']:
            lines.append(f"stack   : {len(facts['stack'])} candidate words (no symbols to resolve them)")
    steps = facts['steps']
    if steps:
        lines.append('steps   : last 25 of %d (t in ms since boot)' % len(steps))
        for t, sub, text_ in steps[-25:]:
            lines.append(f'          {t:>10}  {sub:<14} {text_}')
        if crash or not facts['ended']:
            t, sub, text_ = steps[-1]
            lines.append(f'last step before the end: [{sub}] {text_}')
    return '\n'.join(lines), facts


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('journal', type=pathlib.Path)
    parser.add_argument('--symbols', type=pathlib.Path, help='directory of archived builds (symbols/<build-id>/llvm-pie.elf)')
    parser.add_argument('--entry-symbol', default='_start', help='symbol whose address the title recorded as entry')
    parser.add_argument('--json', action='store_true', help='print the parsed facts instead of the report')
    args = parser.parse_args()
    text = args.journal.read_text(errors='replace')
    summary, facts = report(text, args.symbols, args.entry_symbol)
    print(json.dumps(facts, indent=1, default=str) if args.json else summary)


if __name__ == '__main__':
    sys.exit(main())
