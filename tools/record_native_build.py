#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Record an existing private native build for exact diagnostics and updates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from analyze_session import elf_layout
from build_native_game_bundle_updater import inventory, inventory_hash
from report_diagnostic_session import read_regular


def record(game):
    if not game.is_absolute() or game.is_symlink() or not game.is_dir():
        raise ValueError('Use the ordinary absolute completed GAME_WORK directory')
    private = game/'private'
    if private.is_symlink() or not private.is_dir():
        raise ValueError('Missing ordinary private output directory')
    app = private/'native/PPSA94678'
    files = inventory(app)
    inputs = json.loads(read_regular(game/'native-link/native-inputs.json', 8*1024*1024))
    if not isinstance(inputs, dict):
        raise ValueError('Invalid native input record')
    build = inputs.get('build_id', '')
    if not isinstance(build, str) or not re.fullmatch(r'integrated-[a-f0-9]{24}', build):
        raise ValueError('Missing generated native build fingerprint')
    symbols = read_regular(game/'native-link/llvm-pie.elf', 512*1024*1024)
    elf_layout(symbols)
    executable = read_regular(app/'eboot.bin', 128*1024*1024)
    if build.encode() not in symbols or build.encode() not in executable:
        raise ValueError('ELF/executable does not contain the recorded build identity')
    spec = {'schema': 1, 'title_id': 'PPSA94678', 'build_id': build,
            'symbol_elf_sha256': hashlib.sha256(symbols).hexdigest(),
            'eboot_sha256': hashlib.sha256(executable).hexdigest(),
            'app_manifest_sha256': inventory_hash(files), 'files': files,
            'file_count': len(files), 'app_relative_to_game_work': 'private/native/PPSA94678',
            'symbols_relative_to_game_work': 'native-link/llvm-pie.elf',
            'console_executed': False,
            'scope': 'Private local content receipt, not proof of console execution or reproducible binary equivalence'}
    payload = (json.dumps(spec, indent=2)+'\n').encode()
    output = private/'CANDIDATE.json'
    if output.exists() or output.is_symlink():
        if read_regular(output, 1024*1024) != payload:
            raise ValueError('Previous build receipt differs; preserve it and use a separate new GAME_WORK')
    else:
        with os.fdopen(os.open(output, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600), 'wb') as stream:
            stream.write(payload)
    return output, spec


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_work', type=Path)
    args = parser.parse_args()
    try:
        output, spec = record(args.game_work)
    except (ValueError, KeyError, OSError) as error:
        parser.error(str(error))
    print('Recorded private build:', output, spec['build_id'])
    print('Keep this receipt, native application and matching native-link/llvm-pie.elf private and together.')


if __name__ == '__main__':
    main()
