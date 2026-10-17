#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run real embedded HDR shaders off-screen on host Vulkan (never a PS5 test)."""
import argparse,os,pathlib,shlex,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--headers',type=pathlib.Path);a=p.parse_args()
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','vulkan'],text=True));env=dict(os.environ)
# CI prefers the software driver; an existing host GPU is used only if no LVP is installed.
lvp=sorted(pathlib.Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if lvp:env['VK_ICD_FILENAMES']=str(lvp[0])
with tempfile.TemporaryDirectory(prefix='dkrr-hdr-vulkan-') as d:
 out=pathlib.Path(d)/'test';cmd=['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers','-I'+str(r/'src')]
 if a.headers:cmd+=['-I'+str(a.headers)]
 subprocess.run(cmd+[str(r/'tests/ps5_hdr_vulkan.cpp'),*flags,'-o',str(out)],check=True)
 subprocess.run([str(out)],env=env,check=True,timeout=60)
