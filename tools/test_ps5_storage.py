#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test final PS5 storage sources against host-built upstream support libraries.

Only hard_link_count is modelled: 1 -> 0 (native metadata), actual hard links
retain their count and a designated path raises a metadata error. Cleanup,
verification, leases, startup scheduling and persistence run unmodified.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('work', type=Path, help='update work with native-rebase and prepared2')
args = parser.parse_args()
work = args.work.resolve()
source = work / 'prepared2/source'
host = source / '_release-current/build/host'
game = source / '_release-current/runtime-recomp/src/game'
final = work / 'native-rebase/runtime-recomp/src/game'
deps = source / 'DKR-R/extern'

with tempfile.TemporaryDirectory(prefix='ps5-storage-', dir=work) as folder:
    tmp = Path(folder)
    seam = tmp / 'metadata.hpp'
    seam.write_text('''#include <filesystem>
#include <stdexcept>
inline std::uintmax_t fixture_link_count(const std::filesystem::path& path) {
    if(path.filename().string()==std::string(64,'c'))
        throw std::filesystem::filesystem_error("fixture metadata error",path,std::make_error_code(std::errc::io_error));
    const auto count=std::filesystem::hard_link_count(path);
    return count==1?0:count;
}
''')
    sources = []
    for name in ('maintenance.cpp', 'service.cpp'):
        text = (final / 'storage' / name).read_text()
        # Each platform branch contains one call; preserve the conditional and
        # exercise __PROSPERO__, not a new implementation of the comparison.
        if text.count('fs::hard_link_count(p)') != 2:
            raise SystemExit('Storage metadata seam changed: ' + name)
        path = tmp / name
        path.write_text(text.replace('fs::hard_link_count(p)', 'fixture_link_count(p)'))
        sources.append(str(path))
    # The lock and durable write that every PS5 persistence path goes through:
    # compile the final PS5 sources, not the host library's copy of them.
    for name in ('update_process.cpp', 'update_core.cpp'):
        text = (final / 'updates' / name).read_text().replace('@DKRR_SOURCE@', str(source / 'DKR-R'))
        path = tmp / name
        path.write_text(text)
        sources.append(str(path))
    libraries = ['libDKRUpdateInstall.a', 'libDKRUpdateCore.a', 'libDKRUpdateEd25519.a',
                 'libDKRNetplayCrypto.a', 'libDKRUpdateZip.a']
    hash_library = host / 'libDKRUpdateHash.a'
    if not hash_library.exists():
        hash_library = host / 'mbedtls/library/libmbedcrypto.a'
    compile_row = next(row for row in json.loads((host / 'compile_commands.json').read_text())
                       if Path(row['file']) == game / 'storage/maintenance.cpp')
    # Use the same Mbed TLS layout as the linked provider, not a test-only config.
    hash_defines = [flag for flag in shlex.split(compile_row['command']) if flag.startswith('-DMBEDTLS')]
    command = ['clang++', '-std=c++20', '-O1', '-g', '-D__PROSPERO__',
               *hash_defines,
               '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
               '-include', str(seam), '-I' + str(game), '-I' + str(game / 'storage'),
               '-I' + str(game / 'updates'), '-I' + str(host / 'n64-modern-runtime/librecomp/miniz'), '-I' + str(deps / 'n64-modern-runtime/thirdparty/miniz'), '-I' + str(deps / 'monocypher/src/optional'), '-I' + str(deps / 'monocypher/src'), '-I' + str(deps / 'n64-modern-runtime/thirdparty'),
               '-I' + str(deps / 'mbedtls/include'), *sources, str(ROOT / 'tests/ps5_storage.cpp'),
               '-Wl,--start-group', *[str(host / name) for name in libraries], str(hash_library), '-Wl,--end-group',
               '-pthread', '-o', str(tmp / 'test')]
    # Shipped configuration first, then the unattended worker the console does not run yet.
    # "shipped-ps5env" runs the shipped code with the observed PS5 file-system
    # behaviour and the tolerance rules forced (tests/ps5_env_shim.c, docs/PS5_FACTS.md).
    shim = ROOT / 'tests/ps5_env_shim.c'
    # The shipped step markers go to the real crash-safe journal code.
    diagnostics = tmp / 'diagnostics.o'
    subprocess.run(['clang', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-I' + str(ROOT / 'src'),
                    '-DDKR_DIAG_DATA_ROOT="' + str(tmp / 'journal-root') + '"', '-c',
                    str(ROOT / 'src/ps5_diagnostics.c'), '-o', str(diagnostics)], check=True)
    for name, extra, shimmed in (('shipped', [], False), ('shipped-ps5env', [], True)):
        link = [str(diagnostics), *([str(shim), '-ldl'] if shimmed else [])]
        subprocess.run([*command[:-2], *link, '-o', str(tmp / name), *extra], check=True)
        env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1'}
        if shimmed:
            env['DKRR_PS5_ENV'] = 'all'
        subprocess.run([str(tmp / name), str(tmp / ('fixture-' + name))], check=True, timeout=60, env=env)
