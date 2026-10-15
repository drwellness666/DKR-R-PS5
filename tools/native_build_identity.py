#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fingerprint native inputs locally; this private inventory is never published."""
import argparse,hashlib,json,pathlib
def identify(root,game,sdl,support,high):
    groups={'source':root/'src','patches':root/'patches','game':game/'game-ps5',
            'native':support/'tooling/native','sdl':sdl/'build/sdl',
            'samplerate':sdl/'build/samplerate'}
    rows=[]
    for name,folder in groups.items():
        for file in sorted(folder.rglob('*')):
            if not file.is_file():continue
            if name in ('game','sdl','samplerate') and file.suffix!='.a' and file.name not in ('dkr-ps5-profile.json','dkr-release.json'):continue
            if name in ('source','patches','native') and file.suffix not in ('.c','.cpp','.h','.hpp','.inl','.patch','.json'):continue
            rows.append({'path':name+'/'+str(file.relative_to(folder)),'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    for relative in ('VULKAN_SOURCES.json','tools/link_native_game.sh','tools/native_build_identity.py','tools/generate_native_import_check.py'):
        file=root/relative;rows.append({'path':relative,'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    gpu=support.parent
    dependencies=[gpu/'radv-build/src/amd/vulkan/libvulkan_radeon.a']+sorted((support/'.deps/native/ps5-payload-sdk/target/lib').glob('*'))
    for file in dependencies:
        if file.is_file():rows.append({'path':'gpu/'+str(file.relative_to(gpu)),'size':file.stat().st_size,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    record={'schema':1,'high_refresh_build':high,'inputs':rows}
    digest=hashlib.sha256(json.dumps(record,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    record['build_id']='integrated-'+digest[:24]
    return record
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('root','game','sdl','support','output'):p.add_argument(arg,type=pathlib.Path)
    p.add_argument('--high-refresh',type=int,choices=(0,1),required=True);a=p.parse_args()
    result=identify(a.root,a.game,a.sdl,a.support,a.high_refresh)
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(result['build_id'])
