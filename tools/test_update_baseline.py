#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exact predecessor/output policy tests; no rebuild, network or console access."""
import json,pathlib,tempfile
from check_update_baseline import baseline_output
from build_native_game_bundle_updater import inventory,inventory_hash
with tempfile.TemporaryDirectory(prefix='dkrr-update-baseline-') as temp:
    app=pathlib.Path(temp)/'PPSA94678';(app/'sce_sys').mkdir(parents=True);(app/'sce_module').mkdir()
    (app/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+b'synthetic-code');(app/'sce_module/libc.prx').write_bytes(b'synthetic-module')
    def metadata(value):(app/'sce_sys/param.json').write_text(json.dumps(value)+'\n')
    def digest():return inventory_hash(inventory(app,previous=True))
    for attribute,result in ((0,0),(0x40040,1)):
        metadata({'titleId':'PPSA94678','attribute3':attribute});expected=digest();assert baseline_output(app,expected)==result
        try:baseline_output(app,'0'*64)
        except ValueError:pass
        else:raise AssertionError('Wrong predecessor digest accepted')
    for value in (True,0x80000,'262208',0x40041):
        metadata({'titleId':'PPSA94678','attribute3':value});expected=digest()
        try:baseline_output(app,expected)
        except ValueError:pass
        else:raise AssertionError('Unqualified output declaration accepted')
    metadata({'titleId':'PPSA94678','attribute3':0});(app/'sce_sys/param.json').write_text('{"titleId":"PPSA94678","attribute3":0,"attribute3":262208}')
    expected=digest()
    try:baseline_output(app,expected)
    except ValueError:pass
    else:raise AssertionError('Duplicate metadata key accepted')
    print('PASS exact predecessor preflight and preserved ordinary/capable output declaration; mismatch/unknown/bool/duplicate refusal')
