#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual common-setting paths and read-only pack discovery; no console."""
import pathlib,os,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-shared-profiles-') as tmp:
 p=pathlib.Path(tmp);binary=p/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'src'),str(root/'tests/ps5_shared_profiles.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(p/'own')],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
