#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise native RTC entropy adapter; no network or console connection."""
import os,pathlib,signal,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-rtc-platform-') as folder:
 out=pathlib.Path(folder)/'test'
 common=['clang','-D_GNU_SOURCE','-std=c11','-O1','-Wall','-Wextra','-Werror','-Wl,--wrap=open','-Wl,--wrap=read','-Wl,--wrap=fstat','-Wl,--wrap=close','-pthread',str(r/'src/ps5_rtc_platform.c'),str(r/'tests/ps5_rtc_platform.c')]
 subprocess.run(common+['-fsanitize=address,undefined','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
 for mode in ('device','device-error','regular','eof','kernel','kernel-error','kernel-eof'):
  subprocess.run([str(out),mode],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
 subprocess.run(common+['-o',str(out)],check=True)
 failed=subprocess.run([str(out),'failure'],capture_output=True)
 assert failed.returncode==-signal.SIGABRT and b'refusing insecure ICE credentials' in failed.stderr
 print('PASS original void ICE API fails closed when the native secure RNG fails')
