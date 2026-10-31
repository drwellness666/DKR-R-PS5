#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the native socket adapter with real host I/O behind SceNet-shaped IDs."""
import os,subprocess,tempfile
from pathlib import Path
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-scenet-') as t:
 p=Path(t)/'test'
 subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wno-macro-redefined','-Wall','-Wextra','-Werror','-O1','-g','-fsanitize=address,undefined','-I'+str(r/'src'),str(r/'src/ps5_net.c'),str(r/'tests/ps5_scenet_host.c'),str(r/'tests/ps5_native_net.c'),'-Wl,--wrap=getifaddrs','-pthread','-o',str(p)],check=True)
 subprocess.run([str(p)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1'),check=True,timeout=30)
