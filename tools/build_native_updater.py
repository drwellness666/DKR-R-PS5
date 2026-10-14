#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,pathlib,subprocess,sys

if len(sys.argv) not in (2,3):raise SystemExit('Usage: build_native_updater.py SDK_V044_ROOT [NEXT_VERSION]')
version=int(sys.argv[2]) if len(sys.argv)==3 else 4
if not 4<=version<=99:raise SystemExit('Unsupported update version')
root=pathlib.Path(__file__).resolve().parent.parent
sdk=pathlib.Path(sys.argv[1]).resolve()
work=root/'work/vulkan/native-updater';work.mkdir(parents=True,exist_ok=True)
source=root/'work/vulkan/PS5_PayloadSDK/crt/nid.c'
# Reuse the public SDK's existing streaming SHA-1; retain its GPL notice.
original=source.read_text()
original=original.replace('nid_encode(', 'dkrr_update_unused_nid_encode(')
original+='''\n/* Project integration: hash file bytes without retaining a file copy. */
extern long read(int, void *, unsigned long);
int dkrr_sha1_fd(int fd, unsigned char digest[20]) {
    SHA1_CTX context; unsigned char bytes[8192]; long got;
    SHA1Init(&context);
    while ((got=read(fd,bytes,sizeof(bytes)))>0) SHA1Update(&context,bytes,(unsigned int)got);
    if (got<0) return -1;
    SHA1Final(digest,&context); return 0;
}
'''
(work/'sdk_sha1_file.c').write_text(original)
files=[('eboot.bin','eboot.bin',0o755),('sce_sys/param.json','param.json',0o644)]
header='struct update_file { const char *path; const unsigned char *start; unsigned long long old_size,new_size; const char *old_sha1,*new_sha1; unsigned mode; };\n'
assembly='.section .rodata\n';entries=[]
for i,(path,old_name,mode) in enumerate(files):
    old=root/f'work/vulkan/native-title/previous-v{version-1}'/old_name
    new=root/'native/PPSA94677'/path
    old_bytes,new_bytes=old.read_bytes(),new.read_bytes()
    if any(c in str(new) for c in '\n\r"\\'):raise SystemExit('Unsupported path')
    header+=f'extern const unsigned char native_update_{i}_start[];\n'
    assembly+=f'.balign 16\n.global native_update_{i}_start\nnative_update_{i}_start:\n.incbin "{new}"\n'
    entries.append(f'{{"/data/homebrew/PPSA94677/{path}",native_update_{i}_start,{len(old_bytes)},{len(new_bytes)},"{hashlib.sha1(old_bytes).hexdigest()}","{hashlib.sha1(new_bytes).hexdigest()}",{mode:#o}}}'.replace('0o','0'))
header+='static const struct update_file update_files[]={\n'+',\n'.join(entries)+'\n};\n'
(work/'native_update_files.h').write_text(header);(work/'embedded.S').write_text(assembly)
cc=str(sdk/'bin/prospero-clang')
for file in ['sdk_sha1_file.c','embedded.S']:
    subprocess.run([cc,'-O2','-c',str(work/file),'-o',str(work/(file+'.o'))],check=True)
output=root/f'bin/dkrr-update-native-gpu-v{version}.elf'
subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DDKR_UPDATE_VERSION={version}','-I',str(work),str(root/'src/update_native_probe.c'),str(work/'sdk_sha1_file.c.o'),str(work/'embedded.S.o'),'-o',str(output)],check=True)
print('Built exact-old-build updater:',output)
