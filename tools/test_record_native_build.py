#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise private build receipts with a real symbol ELF and updater inventory."""
import json
import pathlib
import stat
import subprocess
import tempfile

from record_native_build import record

with tempfile.TemporaryDirectory(prefix='dkrr-build-receipt-') as temp:
    game = pathlib.Path(temp)/'game'
    app = game/'private/native/PPSA94678'
    (app/'sce_sys').mkdir(parents=True)
    (app/'sce_module').mkdir()
    link = game/'native-link'
    link.mkdir()
    build = 'integrated-'+'a'*24
    (link/'program.c').write_text('const char identity[]="'+build+'"; int main(void){return identity[0]==0;}\n')
    subprocess.run(['clang', '-g', '-fPIE', '-pie', str(link/'program.c'), '-o', str(link/'llvm-pie.elf')], check=True)
    (link/'native-inputs.json').write_text(json.dumps({'build_id': build}))
    executable = bytes.fromhex('4f153d1d')+build.encode()
    (app/'eboot.bin').write_bytes(executable)
    (app/'sce_module/libc.prx').write_bytes(b'local synthetic module')
    (app/'sce_sys/param.json').write_text(json.dumps({'titleId': 'PPSA94678'}))
    for name in ('icon0.png', 'pic0.dds', 'pic1.dds'):
        (app/'sce_sys'/name).write_bytes(b'synthetic artwork')
    output, spec = record(game)
    assert spec['file_count'] == 6 and spec['build_id'] == build
    assert stat.S_IMODE(output.stat().st_mode) == 0o600 and not spec['console_executed']
    before = output.read_bytes()
    assert record(game)[1] == spec
    def refuses():
        try:
            record(game)
        except ValueError:
            assert output.read_bytes() == before
            return
        raise AssertionError('Changed/mismatching input accepted')
    (app/'eboot.bin').write_bytes(executable+b'new bytes')
    refuses()
    (app/'eboot.bin').write_bytes(executable)
    (link/'native-inputs.json').write_text(json.dumps({'build_id': 'integrated-'+'b'*24}))
    refuses()
    (link/'native-inputs.json').write_text(json.dumps({'build_id': build}))
    (link/'llvm-pie.elf').write_bytes(b'not an ELF')
    refuses()
print('PASS real ELF identity and full updater inventory, private0600 receipt, repeat acceptance and preserved predecessor on changed/mismatched/invalid inputs')
