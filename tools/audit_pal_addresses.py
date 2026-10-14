#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Audit PAL address candidates against both existing US tables and pinned public symbols.

No ROM is read and no runtime support is enabled. Addresses alone are not a port.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tarfile

def audit(game):
    root = Path(__file__).resolve().parent.parent
    spec = next(row for row in json.loads((root / 'GAME_SOURCES.json').read_text())['sources']
                if row.get('name') == 'dkr-decomp')
    archive = game / 'downloads' / spec['file']
    if hashlib.sha256(archive.read_bytes()).hexdigest() != spec['sha256']:
        raise ValueError('Pinned decompilation source archive mismatch')
    maps = {}
    with tarfile.open(archive) as source:
        members = source.getmembers()
        for identity in ('us.v77', 'us.v80', 'pal.v77', 'pal.v80'):
            name = f'ver/symbols/symbol_addrs.{identity}.txt'
            matching = [row for row in members if row.isfile() and row.name.endswith('/' + name)]
            if len(matching) != 1:
                raise ValueError('Missing or ambiguous pinned symbol source: ' + name)
            data = source.extractfile(matching[0]).read()
            if data != (game / 'source/dkr-decomp' / name).read_bytes():
                raise ValueError('Prepared symbol source changed: ' + name)
            maps[identity] = {key: int(value, 16) for key, value in re.findall(
                r'^([A-Za-z_][A-Za-z_0-9]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;', data.decode(), re.M)}
    header = (game / 'source/DKR-R/runtime-recomp/src/game/revision_addresses.hpp').read_text()
    structure = re.search(r'struct AddressTable\s*\{(.*?)\};', header, re.S).group(1)
    fields = re.findall(r'std::uint32_t\s+(\w+)\s*;', structure)
    tables = []
    for label in ('kUsV77', 'kUsV80'):
        body = re.search(label + r'\s*\{(.*?)\};', header, re.S).group(1)
        tables.append([int(value, 16) for value in re.findall(r'0x([0-9A-Fa-f]+)U', body)])
    if not fields or len(fields) != len(tables[0]) or len(fields) != len(tables[1]):
        raise ValueError('Runtime address structure/table sizes disagree')
    rows = []
    for field, old, revised in zip(fields, *tables):
        candidates = [name for name, address in maps['us.v77'].items()
                      if address == old and maps['us.v80'].get(name) == revised
                      and name in maps['pal.v77'] and name in maps['pal.v80']]
        row = {'field': field, 'us_v77': hex(old), 'us_v80': hex(revised),
               'cross_verified_candidates': candidates}
        if len(candidates) == 1:
            name = candidates[0]
            row.update(symbol=name, pal_v77=hex(maps['pal.v77'][name]), pal_v80=hex(maps['pal.v80'][name]))
        rows.append(row)
    return {'schema': 1, 'decomp_revision': spec['revision'], 'fields': len(rows),
            'mapped': sum('symbol' in row for row in rows), 'rom_data_used': False,
            'pal_runtime_enabled': False, 'rows': rows,
            'limits': ['Public symbol correspondence, not matching PAL ELF or hook verification.',
                       'No PAL CPU payload, assets, frame-rate or gameplay acceptance is asserted.']}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_work', type=Path)
    args = parser.parse_args()
    if not args.game_work.is_absolute():
        parser.error('Use an absolute prepared game work directory')
    print(json.dumps(audit(args.game_work), indent=2))
