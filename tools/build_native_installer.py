#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import pathlib,subprocess,sys

if len(sys.argv)!=2:
    raise SystemExit('Usage: build_native_installer.py SDK_ROOT')
root=pathlib.Path(__file__).resolve().parent.parent
sdk=pathlib.Path(sys.argv[1]).resolve()
app=root/'native/PPSA94677'
work=root/'work/vulkan/native-installer'
work.mkdir(parents=True,exist_ok=True)
files=['sce_module/libc.prx','sce_sys/icon0.png','eboot.bin','sce_sys/param.json']
assembly='.section .rodata\n'
header='struct native_file { const char *path; const unsigned char *start,*end; unsigned mode; };\n'
for i,name in enumerate(files):
    path=app/name
    if not path.is_file():raise SystemExit('Missing '+str(path))
    if any(c in str(path) for c in '\n\r"\\'):raise SystemExit('Unsupported path')
    assembly+=f'.balign 16\n.global native_file_{i}_start\n.global native_file_{i}_end\nnative_file_{i}_start:\n.incbin "{path}"\nnative_file_{i}_end:\n'
    header+=f'extern const unsigned char native_file_{i}_start[],native_file_{i}_end[];\n'
header+='static const struct native_file native_files[]={\n'
for i,name in enumerate(files):
    mode='0755' if name in ('eboot.bin','sce_module/libc.prx') else '0644'
    header+=f'{{"/data/homebrew/PPSA94677/{name}",native_file_{i}_start,native_file_{i}_end,{mode}}},\n'
header+='};\n'
(work/'embedded.S').write_text(assembly)
(work/'native_probe_files.h').write_text(header)
cc=str(sdk/'bin/prospero-clang')
subprocess.run([cc,'-c',str(work/'embedded.S'),'-o',str(work/'embedded.o')],check=True)
subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(work),str(root/'src/install_native_probe.c'),str(work/'embedded.o'),'-o',str(root/'bin/dkrr-install-native-gpu.elf')],check=True)
print('Built',root/'bin/dkrr-install-native-gpu.elf')
