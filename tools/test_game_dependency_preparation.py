#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare all pinned public game dependencies below an outer Git repository."""
import argparse,hashlib,json,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-real-dependencies-') as folder:
    outer=pathlib.Path(folder);subprocess.run(['git','init','--quiet',str(outer)],check=True)
    before={str(p.relative_to(outer/'.git')):hashlib.sha256(p.read_bytes()).hexdigest() for p in (outer/'.git').rglob('*') if p.is_file()}
    work=outer/'work/game';cmd=['python3',str(root/'tools/prepare_game_sources.py'),str(work)]
    for cache in a.cache:cmd+=['--cache',str(cache)]
    subprocess.run(cmd,check=True)
    source=work/'source';dk=source/'DKR-R';receipts=json.loads((source/'dependency-patch-receipts.json').read_text())
    selected={'N64ModernRuntime','N64Recomp','sljit','RT64','hlslpp','plume','nativefiledialog-extended'}
    manifest=json.loads((dk/'patches/manifest.json').read_text());expected=[row['sha256'] for dep in manifest['dependencies'] if dep['name'] in selected for row in dep['patches']]
    assert len(receipts)==54 and [r['patch_sha256'] for r in receipts]==expected
    assert all(r['files'] and all(f['before_sha256']!=f['after_sha256'] for f in r['files']) for r in receipts)
    events=(source/'N64ModernRuntime/ultramodern/src/events.cpp').read_text();assert 'rdram_snapshot' in events and 'presentation_allowed' in events
    recomp=(source/'N64Recomp/include/recomp.h').read_text();assert 'struct recomp_context' in recomp
    assert (source/'N64ModernRuntime/N64Recomp/include/recomp.h').read_bytes()==(source/'N64Recomp/include/recomp.h').read_bytes()
    after={str(p.relative_to(outer/'.git')):hashlib.sha256(p.read_bytes()).hexdigest() for p in (outer/'.git').rglob('*') if p.is_file()};assert before==after
    subprocess.run(cmd,check=True) # Complete stamp/inputs/readback reuse check.
    subprocess.run(['python3',str(root/'tools/test_shared_texture_runtime.py'),str(work)],check=True)
    subprocess.run(['python3',str(root/'tools/test_ps5_shared_save_entry.py'),str(work)],check=True)
    subprocess.run(['python3',str(root/'tools/test_ps5_audio_scratch.py'),str(work)],check=True)
    subprocess.run(['python3',str(root/'tools/test_ps5_audio_block.py'),str(work)],check=True)
    print('PASS actual 54 pinned dependency patches inside outer Git, required runtime ABI/snapshot prerequisites, mirrored compiler headers, outer Git preservation and complete source-stamp reuse')
