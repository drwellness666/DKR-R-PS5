#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generated directory relocation must not relax source-content checks."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

from compose_upstream_release import delta
from stage_ps5_release_final import include_locations, relocate_includes

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--work', type=Path, help='also verify every actual reviewed native-rebase input')
args = parser.parse_args()
manifest = json.loads((ROOT / 'patches/dkrr-109-final-manifest.json').read_text())
recorded = manifest['include_directories']
assert recorded
sha = lambda raw: hashlib.sha256(raw).hexdigest()
shifted = {owner: sha(('different-workspace/' + owner).encode())[:16] for owner in recorded}

with tempfile.TemporaryDirectory(prefix='release-relocation-', dir=ROOT.parent) as folder:
    folder = Path(folder)
    maps = []
    for name in ('first', 'second'):
        source, build = folder / name / 'source/DKR-R', folder / name / 'build'
        receipt = build / 'generated/online-performance/header_receipts.json'
        receipt.parent.mkdir(parents=True)
        origin = source / 'runtime-recomp/src/game/example.hpp'
        directory = sha(str(origin).encode())[:16]
        receipt.write_text(json.dumps([{'path': str(origin), 'new': str(build / 'generated/online-performance/includes' / directory / origin.name)}]))
        maps.append(include_locations(source, build))
    assert maps[0].keys() == maps[1].keys() and maps[0] != maps[1]
    before = '#include "@DKRR_BUILD@/generated/online-performance/includes/' + next(iter(maps[0].values())) + '/example.hpp"\nint keep=1;\n'
    moved = relocate_includes(before, maps[0], maps[1])
    assert moved != before and relocate_includes(moved, maps[1], maps[0]) == before
    assert relocate_includes(moved.replace('keep=1', 'keep=2'), maps[1], maps[0]) != before
    assert relocate_includes(before, {}, maps[1]) == before

if args.work:
    raw = (ROOT / 'patches/dkrr-109-native-final.patch').read_bytes()
    assert sha(raw) == manifest['patch_sha256']
    records = list(re.finditer(rb'^--- a/([^\n]+)\n', raw, re.M))
    count = 0
    for index, record in enumerate(records):
        logical = record[1].decode()
        before = (args.work / 'native-rebase' / (logical + '.upstream')).read_text()
        patch = raw[record.start():records[index + 1].start() if index + 1 < len(records) else len(raw)]
        simulated = relocate_includes(before, recorded, shifted)
        restored = relocate_includes(simulated, shifted, recorded)
        assert restored == before
        row = manifest['files'][logical]
        assert sha(restored.encode()) == row['original_sha256'], logical
        after = delta(restored.encode(), patch)
        assert sha(after) == row['patched_sha256'], logical
        assert sha((restored + '\n// changed code\n').encode()) != row['original_sha256']
        count += 1
    print(f'PASS {count} exact reviewed source inputs across simulated workspace relocation; byte changes still rejected')
print('PASS generated header ownership is workspace-independent, relocation is reversible and never masks code changes')
