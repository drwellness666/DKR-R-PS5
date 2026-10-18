#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise exact notice verification and stale application refusal."""
import json
import shutil
import tempfile
from pathlib import Path
from verify_component_notices import verify, verify_app

ROOT = Path(__file__).resolve().parents[1]


def refuses(action):
    try:
        action()
    except (ValueError, KeyError):
        return
    raise AssertionError('Changed/incomplete notice input accepted')


with tempfile.TemporaryDirectory(prefix='dkrr-notice-') as folder:
    root = Path(folder) / 'source'
    paths = ('THIRD_PARTY.md', 'licenses/Racing-Banana-NOTICE.md',
             'licenses/COMPONENT-NOTICES.json', 'licenses/COMPONENT-NOTICES.txt',
             'licenses/PLATFORM-NOTICE-SOURCES.json')
    for name in paths:
        p = root / name
        p.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, p)
    count = verify(root)
    assert count >= 57
    text = root / 'licenses/COMPONENT-NOTICES.txt'
    original = text.read_bytes()
    at = original.index(b'Permission is hereby granted')
    text.write_bytes(original[:at] + b'p' + original[at + 1:])
    refuses(lambda: verify(root))
    text.write_bytes(original[:-1])
    refuses(lambda: verify(root))
    text.write_bytes(original + b'Unexpected attribution removal')
    refuses(lambda: verify(root))
    text.write_bytes(original)
    manifest = root / 'licenses/COMPONENT-NOTICES.json'
    record = manifest.read_bytes()
    for change in ('missing', 'duplicate', 'bad_size', 'bad_entry', 'bad_root'):
        obj = json.loads(record)
        if change == 'missing':
            obj['files'].pop()
        elif change == 'duplicate':
            obj['files'].insert(1, obj['files'][0])
        elif change == 'bad_size':
            obj['files'][0]['bytes'] = True
        elif change == 'bad_entry':
            obj['files'][0] = None
        else:
            obj = []
        manifest.write_text(json.dumps(obj))
        refuses(lambda: verify(root))
    manifest.write_bytes(record)
    provenance = root / 'licenses/PLATFORM-NOTICE-SOURCES.json'
    saved = provenance.read_bytes()
    provenance.write_bytes(saved + b' ')
    refuses(lambda: verify(root))
    provenance.write_bytes(saved)
    app = Path(folder) / 'app'
    destination = app / 'assets/licenses/THIRD_PARTY.md'
    destination.parent.mkdir(parents=True)
    expected = b''.join((root / n).read_bytes() for n in paths[:2] + (paths[3],))
    destination.write_bytes(expected)
    assert verify_app(root, app) == count
    # The staged copy can be valid text yet omit the newly supplied notices.
    destination.write_bytes(expected[:expected.index(b'# Original component notices')])
    refuses(lambda: verify_app(root, app))
    destination.unlink()
    backing = Path(folder) / 'notice.txt'
    backing.write_bytes(expected)
    destination.symlink_to(backing)
    refuses(lambda: verify_app(root, app))
    destination.unlink()
    refuses(lambda: verify_app(root, app))
    destination.write_bytes(expected)
    assert verify_app(root, app) == count
print(f'PASS {count} exact original notices, corruption/truncation/provenance/manifest faults and stale/symlink/missing app notice refusal')
