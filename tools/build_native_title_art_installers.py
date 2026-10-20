#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build private art-only update/install payloads for recorded own title metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

TITLE = 'PPSA94678'
DESTINATIONS = ('/user/app/PPSA94678/sce_sys', '/user/appmeta/PPSA94678')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk', type=Path)
    parser.add_argument('gpu_work', type=Path)
    parser.add_argument('app', type=Path)
    parser.add_argument('recorded_metadata', type=Path)
    parser.add_argument('output_directory', type=Path)
    args = parser.parse_args()
    if any(not path.is_absolute() for path in vars(args).values()):
        parser.error('Use absolute paths')
    if 'private' not in args.output_directory.parts or args.output_directory.exists():
        parser.error('Use a new private output directory')
    root = Path(__file__).resolve().parent.parent
    param = (args.app / 'sce_sys/param.json').read_bytes()
    if json.loads(param)['titleId'] != TITLE:
        parser.error('Unexpected own title identity')
    old = {}
    for folder in DESTINATIONS:
        for name in ('param.json', 'icon0.png'):
            path = args.recorded_metadata / folder.lstrip('/') / name
            if path.is_symlink():
                parser.error('Symlink input forbidden')
            old[folder + '/' + name] = path.read_bytes()
        if old[folder + '/param.json'] != param:
            parser.error('Recorded own registered metadata differs from candidate identity')
    for name in ('icon0.png', 'pic0.dds', 'pic1.dds'):
        path = args.app / 'sce_sys' / name
        if path.is_symlink() or not path.is_file():
            parser.error('Missing ordinary current artwork file')
    subprocess.run(['python3', str(root / 'tools/validate_title_art.py'), str(args.app / 'sce_sys')], check=True)
    work = args.output_directory
    work.mkdir(parents=True, mode=0o700)
    sha = (args.gpu_work / 'PS5_PayloadSDK/crt/nid.c').read_text().replace('nid_encode(', 'dkrr_art_unused_nid_encode(')
    sha += '''\n#include <errno.h>
extern long read(int, void *, unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]) {
 SHA1_CTX context; unsigned char bytes[8192]; long got; SHA1Init(&context);
 for(;;){got=read(fd,bytes,sizeof(bytes));if(got<0&&errno==EINTR)continue;
  if(got<0)return -1;if(!got)break;SHA1Update(&context,bytes,(unsigned)got);}
 SHA1Final(digest,&context);return 0;
}
'''
    (work / 'sha1.c').write_text(sha)
    header = 'struct update_file {const char*path;const unsigned char*start;unsigned long long old_size,new_size;const char*old_sha1,*new_sha1;unsigned mode;};\n'
    assembly = '.section .rodata\n'
    rows = []
    manifest = []
    for index, (path, previous) in enumerate(sorted(old.items())):
        name = path.rsplit('/', 1)[1]
        current = (args.app / 'sce_sys' / name).read_bytes()
        symbol = '0'
        if current != previous:
            symbol = f'native_update_{index}_start'
            header += f'extern const unsigned char {symbol}[];\n'
            assembly += f'.balign 16\n.global {symbol}\n{symbol}:\n.incbin "{args.app / "sce_sys" / name}"\n'
        rows.append('{' + json.dumps(path) + f',{symbol},{len(previous)},{len(current)},"{hashlib.sha1(previous).hexdigest()}","{hashlib.sha1(current).hexdigest()}",0644' + '}')
        manifest.append({'path': path, 'old_sha256': hashlib.sha256(previous).hexdigest(), 'new_sha256': hashlib.sha256(current).hexdigest()})
    header += 'static const struct update_file update_files[]={' + ','.join(rows) + '};\n'
    (work / 'native_update_files.h').write_text(header)
    (work / 'update.S').write_text(assembly)
    # Guard both registered param files and install only the own metadata art.
    files = [(folder + '/' + name, args.app / 'sce_sys' / name)
             for folder in DESTINATIONS for name in ('param.json', 'icon0.png', 'pic0.dds', 'pic1.dds')]
    assembly = '.section .rodata\n'
    header = 'struct native_file {const char*path;const unsigned char*start,*end;unsigned mode;};\n'
    rows = []
    for index, (target, source) in enumerate(files):
        if any(c in str(source) for c in '\n\r"\\'):
            parser.error('Unsupported input path')
        assembly += f'.balign 16\n.global native_file_{index}_start,native_file_{index}_end\nnative_file_{index}_start:\n.incbin "{source}"\nnative_file_{index}_end:\n'
        header += f'extern const unsigned char native_file_{index}_start[],native_file_{index}_end[];\n'
        rows.append('{' + json.dumps(target) + f',native_file_{index}_start,native_file_{index}_end,0644' + '}')
    header += 'static const char*native_dirs[]={' + ','.join(json.dumps(folder) for folder in DESTINATIONS) + '};\n'
    header += 'static const struct native_file native_files[]={' + ','.join(rows) + '};\n'
    (work / 'native_game_files.h').write_text(header)
    (work / 'install.S').write_text(assembly)
    source = (root / 'src/install_native_game.c').read_text().replace('DKR-R 1.0.8 native preview installer (private build; no ROM included)', 'DKR-R own registered title artwork only; no game/ROM/save input')
    (work / 'install_art.c').write_text(source)
    cc = str(args.sdk / 'bin/prospero-clang')
    for name in ('sha1.c', 'update.S', 'install.S'):
        subprocess.run([cc, '-O2', '-c', str(work / name), '-o', str(work / (name + '.o'))], check=True)
    subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DDKR_UPDATE_VERSION=19', '-I', str(work),
                    str(root / 'src/update_native_probe.c'), str(work / 'sha1.c.o'), str(work / 'update.S.o'),
                    '-o', str(work / 'update-registered-icon.elf')], check=True)
    subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I', str(work), str(work / 'install_art.c'),
                    str(work / 'install.S.o'), '-o', str(work / 'install-registered-backgrounds.elf')], check=True)
    (work / 'ART-UPDATE-PLAN.json').write_text(json.dumps({'title': TITLE, 'predecessors': manifest,
        'destinations': DESTINATIONS, 'scope': 'own registered title artwork and unchanged identity guards only; no database, game, ROM or save update'}, indent=2) + '\n')
    print('Built private own-title metadata artwork payloads:', work)
    print('No console operation. Recorded old identities/icons required; unknown files refused.')

if __name__ == '__main__':
    main()
