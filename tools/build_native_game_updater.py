#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a private exact-previous-file updater for the DKR-R preview title."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('sdk',type=Path)
parser.add_argument('previous',type=Path)
parser.add_argument('current',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--expected-previous-sha256',required=True)
parser.add_argument('--sequence',type=int,required=True)
args=parser.parse_args()
if not 1<=args.sequence<=9999:
    parser.error('Sequence must be between 1 and 9999')
previous,current=args.previous.resolve(),args.current.resolve()
old,new=previous.read_bytes(),current.read_bytes()
if hashlib.sha256(old).hexdigest()!=args.expected_previous_sha256:
    raise SystemExit('Recorded previous build does not match')
if old[:4]!=bytes.fromhex('4f153d1d') or new[:4]!=old[:4]:
    raise SystemExit('Expected native FSELF executables')
output=args.output.resolve()
work=output.parent/(output.stem+'-source')
work.mkdir(parents=True,exist_ok=True)
gpu=Path(os.environ.get('DKR_GPU_WORK',str(root/'work/vulkan')))
source=gpu/'PS5_PayloadSDK/crt/nid.c'
# Reuse the SDK streaming SHA-1 and preserve its license and attribution.
text=source.read_text().replace('nid_encode(', 'dkrr_update_unused_nid_encode(')
text+='''\nextern long read(int, void *, unsigned long);
int dkrr_sha1_fd(int fd, unsigned char digest[20]) {
 SHA1_CTX context; unsigned char bytes[8192]; long got; SHA1Init(&context);
 while ((got=read(fd,bytes,sizeof(bytes)))>0) SHA1Update(&context,bytes,(unsigned int)got);
 if(got<0)return -1; SHA1Final(digest,&context); return 0;
}
'''
(work/'sdk_sha1_file.c').write_text(text)
if any(c in str(current) for c in '\n\r"\\'):
    raise SystemExit('Unsupported input path')
(work/'embedded.S').write_text(f'.section .rodata\n.balign 16\n.global native_update_0_start\nnative_update_0_start:\n.incbin "{current}"\n')
(work/'native_update_files.h').write_text('struct update_file { const char *path; const unsigned char *start; unsigned long long old_size,new_size; const char *old_sha1,*new_sha1; unsigned mode; };\nextern const unsigned char native_update_0_start[];\nstatic const struct update_file update_files[]={'+f'{{"/data/homebrew/PPSA94678/eboot.bin",native_update_0_start,{len(old)},{len(new)},"{hashlib.sha1(old).hexdigest()}","{hashlib.sha1(new).hexdigest()}",0755}}'+'};\n')
cc=str(args.sdk.resolve()/'bin/prospero-clang')
for file in ['sdk_sha1_file.c','embedded.S']:
    subprocess.run([cc,'-O2','-c',str(work/file),'-o',str(work/(file+'.o'))],check=True)
subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DDKR_UPDATE_VERSION={args.sequence}','-I',str(work),str(root/'src/update_native_probe.c'),str(work/'sdk_sha1_file.c.o'),str(work/'embedded.S.o'),'-o',str(output)],check=True)
print('Built own-title updater:',output)
print('Previous SHA256:',hashlib.sha256(old).hexdigest())
print('New SHA256:',hashlib.sha256(new).hexdigest())
print('Contains private ROM-derived executable code; exclude this ELF from public exports.')
