#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare the native numeric RTC adapter with the host resolver; no network."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-rtc-net-') as folder:
    out=pathlib.Path(folder)/'test'
    subprocess.run(['clang','-std=c11','-D_POSIX_C_SOURCE=200809L','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(r/'src/ps5_rtc_net.c'),str(r/'tests/ps5_rtc_net.c'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
