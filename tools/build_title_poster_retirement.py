#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a file-only, exact-known-art retirement; no console operation."""
import argparse,hashlib,json,pathlib,subprocess
TITLE='PPSA94678'
DESTINATIONS=('/data/homebrew/PPSA94678/sce_sys','/user/app/PPSA94678/sce_sys','/user/appmeta/PPSA94678')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('sdk',type=pathlib.Path);p.add_argument('gpu_work',type=pathlib.Path)
p.add_argument('previous_app',type=pathlib.Path);p.add_argument('output_directory',type=pathlib.Path)
a=p.parse_args();root=pathlib.Path(__file__).resolve().parent.parent
if any(not value.is_absolute() for value in vars(a).values()):p.error('Use absolute paths')
if 'private' not in a.output_directory.parts or a.output_directory.exists():p.error('Use a new private output directory')
system=a.previous_app/'sce_sys';data={}
for name in ('param.json','icon0.png','pic0.dds','pic1.dds'):
 path=system/name
 if path.is_symlink() or not path.is_file():p.error('Missing ordinary recorded artwork: '+name)
 data[name]=path.read_bytes()
if json.loads(data['param.json']).get('titleId')!=TITLE:p.error('Unexpected title identity')
# Only the preceding project's paired image, not unknown splash artwork.
if data['pic1.dds']!=data['pic0.dds']:p.error('Previous launch picture is not the recorded paired home-menu art')
work=a.output_directory;work.mkdir(parents=True,mode=0o700)
header='struct poster_file {const char*path,*backup;unsigned long long size;const char*sha1;};\n'
def entry(folder,name,backup=False):
 path=folder+'/'+name;second=json.dumps(path+'.dkrr-retired-launch-poster') if backup else '0'
 return '{'+json.dumps(path)+','+second+f',{len(data[name])},"{hashlib.sha1(data[name]).hexdigest()}"'+'}'
header+='static const struct poster_file poster_guards[]={'+','.join(entry(folder,name) for folder in DESTINATIONS for name in ('param.json','icon0.png','pic0.dds'))+'};\n'
header+='static const struct poster_file poster_files[]={'+','.join(entry(folder,'pic1.dds',True) for folder in DESTINATIONS)+'};\n'
(work/'native_poster_retirement.h').write_text(header)
sha=(a.gpu_work/'PS5_PayloadSDK/crt/nid.c').read_text().replace('nid_encode(','dkrr_poster_unused_nid_encode(')
sha+='''\n#include <errno.h>
extern long read(int,void*,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]){
 SHA1_CTX c;unsigned char bytes[8192];long n;SHA1Init(&c);
 for(;;){n=read(fd,bytes,sizeof(bytes));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,bytes,(unsigned)n);}
 SHA1Final(digest,&c);return 0;}
''';(work/'sha1.c').write_text(sha)
cc=str(a.sdk/'bin/prospero-clang')
subprocess.run([cc,'-O2','-c',str(work/'sha1.c'),'-o',str(work/'sha1.o')],check=True)
subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(work),str(root/'src/retire_title_poster.c'),str(work/'sha1.o'),'-o',str(work/'retire-own-launch-posters.elf')],check=True)
(work/'RETIREMENT-PLAN.json').write_text(json.dumps(dict(title=TITLE,destinations=DESTINATIONS,guards={name:hashlib.sha256(data[name]).hexdigest() for name in data},scope='Exact recorded own title launch-art files only, reversible adjacent backups. No app.db, IPC, process, game code, ROM or save changes.',console_executed=False),indent=2)+'\n')
print('Built private file-only poster retirement:',work/'retire-own-launch-posters.elf')
