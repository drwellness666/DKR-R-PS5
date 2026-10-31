#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reconstruct the frozen 1.0.7 source layers retained by DKR-R 1.0.8.

The upstream release capsule contains first-party deltas and separately hashed
private draw inputs. This step reconstructs source only. Draw inputs are
generated locally later; their bytes are never invented or bundled here.
"""
from pathlib import Path, PurePosixPath
import hashlib
import json
import re


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def safe(name):
    path = PurePosixPath(name)
    if not name or path.is_absolute() or '..' in path.parts or '\\' in name:
        raise ValueError('Unsafe release-layer path')
    return path


def delta(original, patch):
    old = original.decode('utf-8').splitlines(True)
    lines = patch.decode('utf-8').splitlines(True)
    if len(lines) < 3 or not lines[0].startswith('--- ') or not lines[1].startswith('+++ '):
        raise ValueError('Invalid release delta header')
    result, cursor, at = [], 0, 2
    while at < len(lines):
        match = re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@', lines[at])
        if not match:
            raise ValueError('Invalid release delta hunk')
        start = int(match[1]) - (int(match[1]) != 0)
        if not cursor <= start <= len(old):
            raise ValueError('Overlapping release delta')
        result += old[cursor:start]
        cursor, at = start, at + 1
        while at < len(lines) and not lines[at].startswith('@@ '):
            line = lines[at]
            at += 1
            if line.startswith((' ', '-')):
                if cursor >= len(old) or old[cursor] != line[1:]:
                    raise ValueError('Release delta base changed')
                if line[0] == ' ':
                    result.append(old[cursor])
                cursor += 1
            elif line.startswith('+'):
                result.append(line[1:])
            else:
                raise ValueError('Invalid release delta record')
    return ''.join(result + old[cursor:]).encode('utf-8')


def compose(root, expected_manifest, baseline=None):
    root = Path(root)
    baseline = Path(baseline) if baseline is not None else None
    if root.is_symlink() or not root.is_dir():
        raise ValueError('Release source must be an ordinary directory')
    def ordinary(name):
        safe(name)
        current = root
        for part in PurePosixPath(name).parts:
            current /= part
            if current.is_symlink():
                raise ValueError('Release composition cannot follow filesystem links')
        return current
    data = root / 'patches/release-prep/v1.0.7/source-capsule-r2'
    manifest_raw = ordinary(str((data / 'manifest.json').relative_to(root))).read_bytes()
    if digest(manifest_raw) != expected_manifest:
        raise ValueError('Upstream release capsule differs from the source pin')
    manifest = json.loads(manifest_raw)
    if manifest['schema'] != 1 or manifest['version'] != '1.0.7':
        raise ValueError('Unsupported upstream release capsule')
    bases, outputs, verified = {}, {}, 0
    for role, entries in manifest['roles'].items():
        for name, row in entries.items():
            safe(name)
            if 'base' in row:
                safe(row['base'])
                if row['base'] not in bases:
                    bases[row['base']] = ordinary(row['base']).read_bytes().replace(b'\r\n', b'\n')
                raw = bases[row['base']]
                if digest(raw) != row['base_sha256'] and baseline is not None:
                    # New upstream releases retain a frozen capsule whose bases
                    # precede current source. Accept only its original hash.
                    candidate = baseline.joinpath(*safe(row['base']).parts)
                    if candidate.is_symlink():
                        raise ValueError('Capsule baseline cannot follow filesystem links')
                    raw = candidate.read_bytes().replace(b'\r\n', b'\n')
                if digest(raw) != row['base_sha256']:
                    raise ValueError('Release base source changed: ' + row['base'])
                if 'patch' in row:
                    safe(row['patch'])
                    raw = delta(raw, ordinary(str((data / row['patch']).relative_to(root))).read_bytes())
            else:
                safe(row['object'])
                raw = ordinary(str((data / row['object']).relative_to(root))).read_bytes()
            if digest(raw) != row['sha256']:
                raise ValueError('Release-layer reconstruction failed: ' + role + '/' + name)
            verified += 1
            destination = None
            if name.startswith(('src/', 'include/', 'tests/')):
                destination = 'runtime-recomp/' + name
            elif name.startswith(('native/src/', 'native/include/', 'native/tests/')):
                destination = 'runtime-recomp/' + name[len('native/'):]
            elif name.startswith('native/') and PurePosixPath(name).suffix in {'.hpp', '.h', '.cpp', '.inl'}:
                destination = 'runtime-recomp/src/game/' + name[len('native/'):]
            elif name.startswith('renderer/'):
                destination = 'runtime-recomp/ps5-release/' + name
            outputs['runtime-recomp/ps5-release/roles/' + role + '/' + name] = (raw, dict(role=role, source=name, sha256=row['sha256']))
            if destination and role != 'SDK2_BASE':
                outputs[destination] = (raw, dict(role=role, source=name, sha256=row['sha256']))
    # No source mutation until every layer has passed its original byte hash.
    for name in outputs:
        ordinary(name)
    for name, (raw, _) in outputs.items():
        target = ordinary(name)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    receipt = dict(schema=1, upstream_version=(root/'VERSION').read_text().strip() if (root/'VERSION').is_file() else '1.0.7', capsule_version='1.0.7', manifest_sha256=expected_manifest,
                   records_verified=verified,
                   files={name:row for name, (_, row) in sorted(outputs.items())},
                   scope='First-party PS5 source graph; private draw kernels are generated separately')
    (root / 'ps5-release-layers.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--manifest-sha256', required=True)
    args = parser.parse_args()
    result = compose(args.source, args.manifest_sha256)
    print('PASS reconstructed', result['records_verified'], 'upstream layer records;',
          len(result['files']), 'effective PS5 source files')
