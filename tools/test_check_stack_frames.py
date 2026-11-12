#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""A real 64 KiB local buffer must be found, accepted by a baseline, and flagged again when it grows."""
import json,pathlib,subprocess,sys,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
SOURCE=lambda n:'''
#include <string.h>
#include <stdio.h>
__attribute__((noinline)) int big(const char*s){volatile char buffer[%d];memset((void*)buffer,s[0],sizeof(buffer));return buffer[s[1]];}
int main(int argc,char**argv){(void)argc;return big(argv[0])&1;}
'''%n
with tempfile.TemporaryDirectory(prefix='dkrr-frames-') as temp:
    work=pathlib.Path(temp);baseline=work/'baseline.json'
    def build(size):
        (work/'a.c').write_text(SOURCE(size))
        subprocess.run(['clang','-O1','-fno-stack-protector','-o',str(work/'a'),str(work/'a.c')],check=True)
    def check(*extra):
        return subprocess.run([sys.executable,str(root/'tools/check_stack_frames.py'),str(work/'a'),'--baseline',str(baseline),*extra],capture_output=True,text=True)
    build(65536)
    first=check('--update');assert first.returncode==0 and ' big' in first.stdout,first.stdout
    assert json.loads(baseline.read_text())['frames']['big']>=65536
    assert check().returncode==0,'an accepted frame must not fail'
    build(262144)
    grown=check();assert grown.returncode==1 and 'NEW/GROWN' in grown.stdout,grown.stdout
print('PASS stack-frame check: finds a 64 KiB frame, accepts a baseline, flags growth')
