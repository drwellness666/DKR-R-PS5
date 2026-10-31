#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check exact notice bytes and the notices shipped in an application candidate."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath

INTRO = ("# Original component notices\n\n"
         "Original texts from pinned source inputs, including optional and host-only\n"
         "components. Each section retains its own scope and terms. This inventory\n"
         "does not grant font/artwork rights or certify combined-program distribution.\n\n")


def marker(row):
    return ("=" * 72 + "\n" + row['component'] + " — " + row['source'] + "\n" + "=" * 72 + "\n\n").encode()


def verify(root):
    root = Path(root)
    manifest = json.loads((root / 'licenses/COMPONENT-NOTICES.json').read_text())
    if (not isinstance(manifest, dict) or manifest.get('schema') != 2 or
            not isinstance(manifest.get('files'), list) or not 1 <= len(manifest['files']) <= 512):
        raise ValueError('Invalid component-notice manifest')
    catalogue = (root / 'licenses/PLATFORM-NOTICE-SOURCES.json').read_bytes()
    if hashlib.sha256(catalogue).hexdigest() != manifest.get('source_notice_catalogue_sha256'):
        raise ValueError('Changed platform-notice provenance catalogue')
    blob = (root / 'licenses/COMPONENT-NOTICES.txt').read_bytes()
    expected = bytearray(INTRO.encode())
    seen = set()
    components = set()
    for row in manifest['files']:
        if not isinstance(row, dict):
            raise ValueError('Invalid component-notice entry')
        component, source, length, digest = (row[k] for k in ('component', 'source', 'bytes', 'sha256'))
        if (not isinstance(component, str) or not component or '\n' in component or '\r' in component or
                not isinstance(source, str) or not source or '\n' in source or '\r' in source or
                PurePosixPath(source).is_absolute() or '..' in PurePosixPath(source).parts or
                type(length) is not int or not 0 < length <= 8 * 1024 * 1024 or
                not isinstance(digest, str) or len(digest) != 64 or
                any(c not in '0123456789abcdef' for c in digest) or
                (component, source) in seen):
            raise ValueError('Invalid/duplicate component-notice entry')
        seen.add((component, source))
        components.add(component)
        header = marker(row)
        offset = len(expected)
        if blob[offset:offset + len(header)] != header:
            raise ValueError('Missing or changed notice heading: ' + source)
        start = offset + len(header)
        data = blob[start:start + length]
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('Changed or truncated original notice: ' + source)
        expected.extend(header + data + b'\n\n')
    required = {'DKR-R', 'N64ModernRuntime', 'RT64', 're-spirv', 'PS5 Mesa',
                'PS5 native tooling', 'PS5 payload SDK', 'PS5 platform SDK', 'zlib', 'curl', 'Mozilla CA store'}
    if not required <= components:
        raise ValueError('Missing required runtime/build component notices')
    if bytes(expected) != blob:
        raise ValueError('Unexpected notice content or framing')
    return len(seen)


def verify_app(root, app):
    count = verify(root)
    root, app = Path(root), Path(app)
    expected = b''.join((root / name).read_bytes() for name in (
        'THIRD_PARTY.md', 'licenses/Racing-Banana-NOTICE.md', 'licenses/COMPONENT-NOTICES.txt'))
    target = app / 'assets/licenses/THIRD_PARTY.md'
    if target.is_symlink() or not target.is_file() or target.read_bytes() != expected:
        raise ValueError('Application component notices are stale or incomplete; relink the candidate')
    return count


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('root', type=Path)
    p.add_argument('--app', type=Path)
    a = p.parse_args()
    try:
        n = verify_app(a.root, a.app) if a.app else verify(a.root)
    except (ValueError, KeyError, OSError) as error:
        p.error(str(error))
    print(f'PASS {n} exact original component notices' + (' and matching application copy' if a.app else ''))
