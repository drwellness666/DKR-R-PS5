#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a private, collision-refusing installer for the selected owned canonical ROM(s)."""
import argparse, hashlib, json, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('sdk',type=Path);p.add_argument('game_work',type=Path);a=p.parse_args()
sdk,game=a.sdk.resolve(),a.game_work.resolve();private=game/'private';selected=json.loads((private/'roms/selected-revisions.json').read_text())['revisions']
known={77:'0cb115d8716dbbc2922fda38e533b9fe63bb9670',80:'6d96743d46f8c0cd0edb0ec5600b003c89b93755'}
if not selected or any(rev not in known for rev in selected) or len(set(selected))!=len(selected):raise SystemExit('Invalid owned ROM selection')
work=private/'owned-rom-installer';work.mkdir(parents=True,exist_ok=True,mode=0o700)
assembly='.section .rodata\n';header='struct native_file {const char*path;const unsigned char*start,*end;unsigned mode;};\n';rows=[]
for i,rev in enumerate(selected):
 rom=private/'roms'/f'dkr.us.v{rev}.z64'
 if rom.is_symlink() or rom.stat().st_size!=12*1024*1024 or hashlib.sha1(rom.read_bytes()).hexdigest()!=known[rev]:raise SystemExit('Canonical owned ROM failed verification')
 if any(c in str(rom) for c in '\n\r"\\'):raise SystemExit('Unsupported working directory path')
 assembly+=f'.balign 16\n.global native_file_{i}_start,native_file_{i}_end\nnative_file_{i}_start:\n.incbin "{rom}"\nnative_file_{i}_end:\n'
 header+=f'extern const unsigned char native_file_{i}_start[],native_file_{i}_end[];\n'
 rows.append(f'{{"/data/dkrr/roms/dkr.us.v{rev}.z64",native_file_{i}_start,native_file_{i}_end,0644}}')
header+='static const char*native_dirs[]={"/data/dkrr","/data/dkrr/roms"};\nstatic const struct native_file native_files[]={'+','.join(rows)+'};\n'
(work/'embedded.S').write_text(assembly);(work/'native_game_files.h').write_text(header)
# Same byte-for-byte installer guard; source copy changes only its title message.
s=(root/'src/install_native_game.c').read_text().replace('DKR-R 1.0.7 native preview installer (private build; no ROM included)','PRIVATE OWN-ROM INSTALLER: selected owned dumps to own console only').replace('NATIVE INSTALL PASS. Files staged only; mounting and gameplay untested.','OWN ROM INSTALL PASS. Never publish or share this generated ELF.')
(work/'install_owned.c').write_text(s);cc=str(sdk/'bin/prospero-clang');subprocess.run([cc,'-c',str(work/'embedded.S'),'-o',str(work/'embedded.o')],check=True)
output=private/'dkrr-install-owned-rom.elf';subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(work),str(work/'install_owned.c'),str(work/'embedded.o'),'-o',str(output)],check=True);output.chmod(0o600)
print('Private owned-ROM installer:',output)
print('CONTAINS ROM DATA. Keep local; exclude from GitHub, issues, releases, CI and shared build archives.')
print('No upload or console change performed.')
