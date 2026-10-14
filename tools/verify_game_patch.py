#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify the pinned pristine inputs, zero-fuzz patch and every post-patch hash."""
import hashlib,json,pathlib,re,subprocess,sys,tarfile,tempfile
if len(sys.argv)!=2:raise SystemExit('Usage: verify_game_patch.py ABSOLUTE_PINNED_DKR_SOURCE_ARCHIVE')
root=pathlib.Path(__file__).resolve().parent.parent
archive=pathlib.Path(sys.argv[1]);manifest=json.loads((root/'patches/dkrr-patch-manifest.json').read_text())
spec=next(x for x in json.loads((root/'GAME_SOURCES.json').read_text())['sources'] if x['name']=='DKR-R')
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
if digest(archive)!=spec['sha256']:raise SystemExit('Pristine source archive hash mismatch')
patch=root/'patches/0005-dkrr-ps5-native-preview.patch'
if digest(patch)!=manifest['patch_sha256']:raise SystemExit('Authored patch hash mismatch')
names=re.findall(r'^\+\+\+ b/([^\t\n]+)',patch.read_text(),re.M)
if set(names)!=set(manifest['files']) or len(names)!=len(set(names)):
    raise SystemExit('Patch file set differs from reviewed manifest')
with tempfile.TemporaryDirectory(prefix='dkrr-pristine-patch-') as temp:
    temp=pathlib.Path(temp)
    with tarfile.open(archive) as tar:
        for name,row in manifest['files'].items():
            relative=pathlib.PurePosixPath(name)
            if relative.is_absolute() or '..' in relative.parts:raise SystemExit('Unsafe manifest path')
            matches=[m for m in tar.getmembers() if m.isfile() and m.name.endswith('/'+name)]
            if len(matches)!=1:raise SystemExit('Missing/ambiguous pristine source: '+name)
            data=tar.extractfile(matches[0]).read()
            if hashlib.sha256(data).hexdigest()!=row['original_sha256']:raise SystemExit('Pristine file hash mismatch: '+name)
            path=temp/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    command=['patch','--batch','--fuzz=0','-p1','-d',str(temp),'-i',str(patch)]
    subprocess.run(command+['--dry-run'],check=True)
    subprocess.run(command,check=True)
    for name,row in manifest['files'].items():
        if digest(temp/name)!=row['patched_sha256']:raise SystemExit('Patched file hash mismatch: '+name)
print(f'PASS: {len(names)} pinned pristine files; zero-fuzz patch; complete pre/post hashes')
