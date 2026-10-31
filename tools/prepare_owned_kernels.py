#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the complete upstream rollback and model closures from owned inputs."""
import argparse,hashlib,json,os,pathlib,subprocess,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game',type=pathlib.Path);p.add_argument('pipeline',type=pathlib.Path);p.add_argument('--v77-build',type=pathlib.Path,required=True);p.add_argument('--v80-build',type=pathlib.Path,required=True);a=p.parse_args()
game=a.game.resolve();pipeline=a.pipeline.resolve();source=game/'source/_release-current';header=game/'source/N64Recomp/include/recomp.h';rsp=game/'source/DKR-R/runtime-recomp/RecompiledRSP'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for revision,build in ((77,a.v77_build),(80,a.v80_build)):
    normal=pipeline/f'generated-v{revision}';policy=pipeline/f'menu-pipeline-v{revision}.toml';elf=build.resolve()/f'dkr.us.v{revision}.elf'
    for kind,script in (('owned','generate_replay_probe.py'),('model','generate_visual_model_kernel.py')):
        target=game/f'private/{kind}-v{revision}';receipt=target/'PS5-CLOSURE-RECEIPT.json'
        inputs=dict(revision=revision,generator=digest(source/'scripts'/script),header=digest(header),policy=digest(policy),elf=digest(elf),rsp={str(f.relative_to(rsp)):digest(f) for f in sorted(rsp.rglob('*')) if f.is_file()},normal={f.name:digest(f) for f in sorted(normal.iterdir()) if f.suffix in ('.h','.c')})
        if target.exists():
            record=json.loads(receipt.read_text());files={f.name:digest(f) for f in target.iterdir() if f.is_file() and f!=receipt}
            if record['inputs']!=inputs or record['files']!=files:raise ValueError('Owned closure inputs changed; use a fresh build directory')
            continue
        if kind=='owned':args=[str(normal),str(header),str(target),'--revision',str(revision),'--scene-cuts','--audio-services','--input-services','--authored-cpu','--isolate-symbols','--full-scenes','--mod-services','--inline-memory','--rsp-source',str(rsp),'--code-policy-config',str(policy)]
        else:args=['--generated',str(normal),'--header',str(header),'--elf',str(elf),'--revision',str(revision),'--output',str(target)]
        subprocess.run([sys.executable,'-B',str(source/'scripts'/script),*args],check=True,env=dict(os.environ,PYTHONHASHSEED='0'))
        files={f.name:digest(f) for f in target.iterdir() if f.is_file()};receipt.write_text(json.dumps(dict(schema=1,inputs=inputs,files=files),indent=2)+'\n')
print('PASS full-scene rollback and model kernels for both owned US revisions')
