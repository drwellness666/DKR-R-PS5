#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare an exact own-pack copy helper, without embedding it or contacting PS5."""
import argparse,hashlib,json,pathlib,re,subprocess,zipfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('sdk',type=pathlib.Path);p.add_argument('pack',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('--sha-source',type=pathlib.Path,required=True);a=p.parse_args()
if not all(x.is_absolute() for x in (a.sdk,a.pack,a.output,a.sha_source)):p.error('Use absolute paths')
if a.output.exists() or a.output.is_symlink() or 'private' not in a.output.parts or a.output.suffix!='.elf':p.error('Use a new private ELF output')
if a.pack.is_symlink() or not a.pack.is_file() or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*\.zip',a.pack.name) or '..' in a.pack.name:p.error('Use the canonical existing own imported ZIP')
if not 0<a.pack.stat().st_size<=2*1024*1024*1024:p.error('Unexpected pack size')
with zipfile.ZipFile(a.pack) as z:
 for name in z.namelist():
  if pathlib.PurePosixPath(name).is_absolute() or '..' in pathlib.PurePosixPath(name).parts or pathlib.PurePosixPath(name).suffix.lower() in ('.z64','.n64','.v64'):p.error('Unexpected pack path or ROM member')
data=a.pack.read_bytes();size=len(data);sha1=hashlib.sha1(data).hexdigest();sha256=hashlib.sha256(data).hexdigest();del data
a.output.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
sha_bytes=a.sha_source.read_bytes()
if hashlib.sha256(sha_bytes).hexdigest()!='6bd670b05ec4a8816688e18cc4425353c2f764f574d0c7f3db5cc4e8c5b7a9dc':p.error('Pinned public SDK SHA1 source checksum mismatch')
sha=sha_bytes.decode()
start=sha.index('SHA1Final(');end=sha.index('{',start)+1;depth=1
while depth:
 depth+=(sha[end]=='{')-(sha[end]=='}');end+=1
sha=sha[:end]+'''\n#include <errno.h>
extern long read(int,void*,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]){SHA1_CTX c;unsigned char b[262144];long n;SHA1Init(&c);for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}SHA1Final(digest,&c);return 0;}\n'''
sha_path=a.output.with_suffix('.sha1.c');sha_path.write_text(sha)
sha_object=a.output.with_suffix('.sha1.o')
# The SDK's self-contained SHA1 prefix also defines unused C string helpers.
# Limit this conventional suppression to that unchanged third-party object.
subprocess.run([str(a.sdk/'bin/prospero-clang'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-c',str(sha_path),'-o',str(sha_object)],check=True)
subprocess.run([str(a.sdk/'bin/prospero-clang'),'-std=c11','-O2','-Wall','-Wextra','-Werror',
               '-DDKR_PACK_NAME="'+a.pack.name+'"','-DDKR_PACK_SHA1="'+sha1+'"','-DDKR_PACK_BYTES='+str(size)+'ULL',
               str(root/'src/copy_owned_texture_pack.c'),str(sha_object),'-o',str(a.output)],check=True)
record={'schema':1,'pack_name':a.pack.name,'pack_size':size,'pack_sha1':sha1,'pack_sha256':sha256,
        'file':a.output.name,'size':a.output.stat().st_size,'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),
        'console_executed':False,'scope':'Copy existing exact own US77 pack to US80; preserve settings, saves and existing different files; no texture or ROM bytes embedded; close own title before sending'}
a.output.with_suffix('.json').write_text(json.dumps(record,indent=2)+'\n');print('Prepared own-pack copy helper:',a.output,'bytes=',record['size'],'; no pack bytes embedded or console connection')
