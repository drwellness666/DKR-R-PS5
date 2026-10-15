#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only predecessor check and preserved output declaration for rebuilds."""
import argparse,json,pathlib,re
from build_native_game_bundle_updater import inventory,inventory_hash,TITLE
def baseline_output(app,expected):
    if not re.fullmatch(r'[0-9a-f]{64}',expected):raise ValueError('Invalid predecessor manifest digest')
    files=inventory(app,previous=True)
    if inventory_hash(files)!=expected:raise ValueError('Recorded predecessor differs; stop before rebuilding')
    def unique(pairs):
        out={}
        for key,value in pairs:
            if key in out:raise ValueError('Duplicate predecessor metadata key')
            out[key]=value
        return out
    raw=(app/'sce_sys/param.json').read_bytes()
    if len(raw)>65536:raise ValueError('Oversized predecessor metadata')
    metadata=json.loads(raw,object_pairs_hook=unique)
    if metadata.get('titleId')!=TITLE:raise ValueError('Wrong predecessor title')
    attribute=metadata.get('attribute3',0)
    if type(attribute) is not int or attribute not in (0,0x40040):raise ValueError('Unqualified predecessor output declaration')
    return int(attribute==0x40040)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('previous',type=pathlib.Path);p.add_argument('expected_sha256');a=p.parse_args()
    try:result=baseline_output(a.previous,a.expected_sha256)
    except (ValueError,OSError) as e:p.error(str(e))
    print(result)
