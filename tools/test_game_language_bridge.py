#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ROM-free test of actual native register bridge using the pinned public header."""
import hashlib,json,pathlib,subprocess,tarfile,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
row=next(r for r in json.loads((root/'GAME_SOURCES.json').read_text())['sources'] if r.get('name')=='N64Recomp')
archive=root/'vendor'/row['file']
if hashlib.sha256(archive.read_bytes()).hexdigest()!=row['sha256']:
    raise SystemExit('Pinned N64Recomp archive hash mismatch')
with tempfile.TemporaryDirectory(prefix='dkrr-language-bridge-') as temp:
    temp=pathlib.Path(temp)
    with tarfile.open(archive) as tar:
        members=[m for m in tar.getmembers() if m.isfile() and (m.name=='include/recomp.h' or m.name.endswith('/include/recomp.h'))]
        if len(members)!=1:raise SystemExit('Ambiguous official recomp_context header')
        (temp/'recomp.h').write_bytes(tar.extractfile(members[0]).read())
    binary=temp/'test'
    subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-I',str(temp),str(root/'tests/game_language_bridge.cpp'),str(root/'src/ps5_game_language.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
