#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare the actual upstream icon/header as private build inputs, without retouch."""
import argparse,hashlib,json,pathlib,shutil,urllib.request
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_work',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
p.add_argument('--header-cache',type=pathlib.Path)
a=p.parse_args()
if not a.game_work.is_absolute() or not a.output.is_absolute():p.error('Use absolute work/output paths')
spec=json.loads((root/'TITLE_ART_SOURCES.json').read_text())
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
icon=a.game_work/'source/DKR-R'/spec['icon']['path']
if icon.is_symlink() or digest(icon)!=spec['icon']['sha256']:raise SystemExit('Upstream icon input hash mismatch')
if a.output.is_symlink():raise SystemExit('Refusing a symlink output directory')
a.output.mkdir(parents=True,exist_ok=True)
header=a.output/'background-master.png'
if header.is_symlink():raise SystemExit('Refusing a symlink background file')
if header.exists() and digest(header)!=spec['background']['sha256']:
    raise SystemExit('Existing background differs; use a fresh title-art input directory')
if not header.exists():
    if a.header_cache is not None:
        if a.header_cache.is_symlink() or digest(a.header_cache)!=spec['background']['sha256']:
            raise SystemExit('Header cache hash mismatch')
        shutil.copy2(a.header_cache,header)
    else:
        request=urllib.request.Request(spec['background']['url'],headers={'User-Agent':'DKR-R-PS5-source-build'})
        data=urllib.request.urlopen(request,timeout=45).read(16*1024*1024+1)
        if len(data)>16*1024*1024 or hashlib.sha256(data).hexdigest()!=spec['background']['sha256']:
            raise SystemExit('Upstream header download checksum/size mismatch')
        header.write_bytes(data)
target=a.output/'icon-master.png'
if target.is_symlink() or (target.exists() and digest(target)!=spec['icon']['sha256']):
    raise SystemExit('Existing icon differs; use a fresh title-art input directory')
shutil.copy2(icon,target)
(a.output/'SOURCE.json').write_text(json.dumps(spec,indent=2)+'\n')
print('PASS: pinned original DKR-R icon and README header, unchanged source bytes; no ROM input')
