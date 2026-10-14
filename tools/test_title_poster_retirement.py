#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual file-only retirement with synthetic own metadata, no console."""
import hashlib,json,pathlib,subprocess,tarfile,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-poster-') as tmp:
 t=pathlib.Path(tmp);folders=[t/name for name in ('source','registered','appmeta')]
 data={name:('fixture-'+name).encode() for name in ('param.json','icon0.png','pic0.dds','pic1.dds')}
 header='struct poster_file {const char*path,*backup;unsigned long long size;const char*sha1;};\n'
 def row(folder,name,backup=False):
  path=str(folder/name);second=json.dumps(path+'.dkrr-retired-launch-poster') if backup else '0'
  return '{'+json.dumps(path)+','+second+f',{len(data[name])},"{hashlib.sha1(data[name]).hexdigest()}"'+'}'
 header+='static const struct poster_file poster_guards[]={'+','.join(row(f,n) for f in folders for n in ('param.json','icon0.png','pic0.dds'))+'};\n'
 header+='static const struct poster_file poster_files[]={'+','.join(row(f,'pic1.dds',True) for f in folders)+'};\n'
 (t/'native_poster_retirement.h').write_text(header)
 with tarfile.open(root/'vendor/ps5-sdk-source.tar.gz') as archive:sha=archive.extractfile('crt/nid.c').read().decode()
 sha=sha.replace('nid_encode(','dkrr_unused_nid_encode(')+'''\n#include <unistd.h>
#include <errno.h>
int dkrr_sha1_fd(int fd,unsigned char digest[20]){SHA1_CTX c;unsigned char b[8192];ssize_t n;SHA1Init(&c);for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}SHA1Final(digest,&c);return 0;}\n'''
 (t/'sha1.c').write_text(sha)
 subprocess.run(['clang','-D_GNU_SOURCE','-O1','-fsanitize=address,undefined','-c',str(t/'sha1.c'),'-o',str(t/'sha1.o')],check=True)
 subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wall','-Wextra','-Werror','-O1','-fsanitize=address,undefined','-I',str(t),str(root/'src/retire_title_poster.c'),str(t/'sha1.o'),'-o',str(t/'test')],check=True)
 def reset():
  for f in folders:
   f.mkdir(exist_ok=True)
   for p in f.iterdir():p.unlink()
   for n,b in data.items():(f/n).write_bytes(b)
 def run(ok):
  p=subprocess.run([str(t/'test')],capture_output=True,text=True)
  assert (p.returncode==0)==ok,p.stdout+p.stderr
 def untouched():
  for f in folders:
   assert (f/'pic1.dds').read_bytes()==data['pic1.dds']
   assert not (f/'pic1.dds.dkrr-retired-launch-poster').exists()
 reset();run(True)
 for f in folders:
  assert not (f/'pic1.dds').exists()
  assert (f/'pic1.dds.dkrr-retired-launch-poster').read_bytes()==data['pic1.dds']
  for n in ('param.json','icon0.png','pic0.dds'):assert (f/n).read_bytes()==data[n]
 run(True)
 reset();(folders[2]/'param.json').write_bytes(b'foreign identity');run(False);untouched()
 reset();(folders[2]/'pic1.dds').write_bytes(b'unknown poster');run(False)
 for f in folders[:2]:assert (f/'pic1.dds').read_bytes()==data['pic1.dds']
 reset();(folders[1]/'pic1.dds.dkrr-retired-launch-poster').write_bytes(data['pic1.dds']);run(False)
 for f in folders:assert (f/'pic1.dds').read_bytes()==data['pic1.dds']
 reset();(folders[2]/'pic1.dds').unlink();(folders[2]/'pic1.dds').symlink_to(folders[0]/'pic1.dds');run(False)
 for f in folders[:2]:assert (f/'pic1.dds').read_bytes()==data['pic1.dds']
 reset()
 for f in folders:(f/'pic1.dds').unlink()
 run(True)
 print('PASS actual poster retirement: all-folder preflight, exact hashes, no-poster/repeat, identity/unknown/collision/symlink refusal, retained icon/selection and reversible backups')
