#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real source verifier with synthetic sources and no network/ROM."""
import pathlib,tempfile,subprocess,sys,json,hashlib,shutil
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-title-source-') as folder:
    t=pathlib.Path(folder);kit=t/'kit';(kit/'tools').mkdir(parents=True)
    shutil.copy2(root/'tools/prepare_upstream_title_art.py',kit/'tools/prepare_upstream_title_art.py')
    spec=json.loads((root/'TITLE_ART_SOURCES.json').read_text())
    game=t/'game';source=game/'source/DKR-R'/spec['icon']['path'];source.parent.mkdir(parents=True)
    source.write_bytes(b'SYNTHETIC ICON INPUT');header=t/'cached.png';header.write_bytes(b'SYNTHETIC HEADER INPUT')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    spec['icon']['sha256']=sha(source);spec['background']['sha256']=sha(header)
    (kit/'TITLE_ART_SOURCES.json').write_text(json.dumps(spec))
    command=[sys.executable,str(kit/'tools/prepare_upstream_title_art.py'),str(game)]
    def run(output,cache,expected):
        result=subprocess.run(command+[str(output),'--header-cache',str(cache)],capture_output=True,text=True)
        assert (result.returncode==0)==expected,result.stdout+result.stderr
    out=t/'out';run(out,header,True);run(out,header,True)
    assert (out/'icon-master.png').read_bytes()==source.read_bytes()
    assert (out/'background-master.png').read_bytes()==header.read_bytes()
    (out/'background-master.png').write_bytes(b'CHANGED');run(out,header,False)
    bad=t/'bad.png';bad.write_bytes(b'WRONG HASH');run(t/'bad-cache',bad,False)
    source.write_bytes(b'WRONG ICON');run(t/'bad-icon',header,False);assert not (t/'bad-icon').exists()
    source.unlink();source.symlink_to(header);run(t/'linked-icon',header,False)
print('PASS: original title-source hash checks, unchanged copy, repeat, mismatches and symlink refusal; no network/ROM used')
