#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the PS5 sign-in token store against a real directory, including the uid/nlink-redaction path."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-keyring-') as tmp:
    for name,extra in (('plain',[]),('redacted',['-DDKRR_TEST_NATIVE_METADATA'])):
        out=pathlib.Path(tmp)/name
        subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*extra,'-I'+str(r/'src'),str(r/'tests/ps5_online_keyring.cpp'),'-o',str(out)],check=True)
        subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
