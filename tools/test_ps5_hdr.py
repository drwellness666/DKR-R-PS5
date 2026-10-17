#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-hdr-') as d:
 p=pathlib.Path(d);out=p/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-pthread','-Wl,--wrap=fsync','-I'+str(r/'src'),f'-DDKR_DISPLAY_DATA_ROOT="{p}/data"',str(r/'tests/ps5_hdr.cpp'),str(r/'src/ps5_hdr_options.cpp'),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
