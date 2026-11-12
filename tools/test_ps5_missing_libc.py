#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The stand-ins for libc functions the PS5 does not export must behave, not crash."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parent.parent
names=('readlink','symlink','link','fchown','mkstemp','strcasestr')
with tempfile.TemporaryDirectory(prefix='dkrr-missing-libc-') as t:
    t=pathlib.Path(t)
    renames=[f'-D{n}=dkrr_ps5_{n}' for n in names]
    subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*renames,
                    str(r/'src/ps5_missing_libc.c'),str(r/'tests/ps5_missing_libc.c'),'-o',str(t/'test')],check=True)
    out=subprocess.run([str(t/'test'),str(t/'file')],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1'),check=True).stdout
    assert out.strip()=='PASS',out
    # Every name the console reported as unresolved must be defined by the file, with the exact symbol name.
    subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-O1','-c',str(r/'src/ps5_missing_libc.c'),'-o',str(t/'plain.o')],check=True)
    defined=subprocess.run(['nm','--defined-only',str(t/'plain.o')],capture_output=True,text=True,check=True).stdout.split()
    for n in names:assert n in defined,n+' is not defined under its real name'
print('PASS missing-libc stand-ins: unsupported calls fail with errno, mkstemp is exclusive, strcasestr is correct')
