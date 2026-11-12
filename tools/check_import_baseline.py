#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every function the title imports must be classified; a missing one crashes at its first call.

The platform leaves imports it cannot resolve bound to address 0 (observed on
the console: readlink, 1.2.6). The start-up audit lists them (journal steps
`sub=imports`). docs/IMPORTS_BASELINE.json records what the console showed:
  verified          imported and resolved on the console
  missing_accepted  missing on the console and never called (dormant features)
  missing_shimmed   missing on the console; src/ps5_missing_libc.c defines them
Build check (this tool on the linked ELF):
  FAIL  a missing_shimmed name is imported again (the stand-in was lost)
  WARN  an import not in the baseline: new since the last console check; start the
        build once on the console, then `pull_console_debug.py` reports whether it resolved
Console check (--journal): any `imports` step not in missing_accepted/missing_shimmed is a NEW unresolved import.
"""
import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys


def elf_imports(elf):
    readelf = shutil.which('llvm-readelf') or shutil.which('readelf')
    if not readelf:
        sys.exit('Need llvm-readelf or readelf')
    out = subprocess.run([readelf, '--wide', '--dyn-syms', str(elf)], capture_output=True, text=True, check=True).stdout
    names = set()
    for line in out.splitlines():
        fields = line.split()
        if len(fields) >= 8 and fields[3] == 'FUNC' and fields[6] == 'UND':
            name = fields[7].split('@')[0]  # host ELFs carry a version suffix; the console's do not
            if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', name):
                names.add(name)
    return names


def classify(imports, baseline):
    shimmed = set(baseline['missing_shimmed'])
    accepted = set(baseline['missing_accepted'])
    verified = set(baseline['verified'])
    return {
        'lost_shim': sorted(imports & shimmed),
        'new': sorted(imports - shimmed - accepted - verified),
        'accepted_missing': sorted(imports & accepted),
    }


def unexpected_missing(journal_text, baseline):
    """Imports the console reported as unresolved that the baseline does not know."""
    known = set(baseline['missing_accepted']) | set(baseline['missing_shimmed'])
    seen = re.findall(r'DKRR_STEP t=\d+ sub=imports (\S+)', journal_text)
    return sorted(set(seen) - known)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('elf', type=pathlib.Path, nargs='?')
    parser.add_argument('--baseline', type=pathlib.Path, required=True)
    parser.add_argument('--strict', action='store_true', help='treat unverified new imports as failure')
    parser.add_argument('--journal', type=pathlib.Path, help='console journal: report new unresolved imports')
    parser.add_argument('--accept-verified', action='store_true', help='add the current new imports to `verified` (after a console check)')
    args = parser.parse_args()
    baseline = json.loads(args.baseline.read_text())
    status = 0
    if args.journal:
        unexpected = unexpected_missing(args.journal.read_text(errors='replace'), baseline)
        for name in unexpected:
            print(f'NEW UNRESOLVED IMPORT on the console: {name} (a call jumps to address 0)')
        return 1 if unexpected else 0
    result = classify(elf_imports(args.elf), baseline)
    for name in result['lost_shim']:
        print(f'FAIL: {name} is imported again but is missing on the console; src/ps5_missing_libc.c must define it')
        status = 1
    for name in result['new']:
        print(f'WARN: new import not yet checked on the console: {name}')
    print(f"{len(result['accepted_missing'])} accepted-missing imports present (dormant features): {', '.join(result['accepted_missing'])}")
    if result['new'] and args.accept_verified:
        baseline['verified'] = sorted(set(baseline['verified']) | set(result['new']))
        args.baseline.write_text(json.dumps(baseline, indent=1) + '\n')
        print(f"Added {len(result['new'])} import(s) to verified.")
    elif result['new'] and args.strict:
        status = 1
    return status


if __name__ == '__main__':
    sys.exit(main())
