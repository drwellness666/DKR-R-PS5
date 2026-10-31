#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the native descriptor query with real host descriptors and a PTY."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-isatty-test-') as folder:
    out=pathlib.Path(folder)/'test'
    subprocess.run(['clang','-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(r/'src/ps5_isatty.c'),str(r/'tests/ps5_isatty.c'),'-lutil','-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
