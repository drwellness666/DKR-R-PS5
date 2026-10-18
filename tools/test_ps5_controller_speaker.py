#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual app speaker worker with synthetic PCM and mocked native AudioOut."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-speaker-') as tmp:
    out=pathlib.Path(tmp)/'speaker'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src'),str(r/'tests/ps5_controller_speaker.cpp'),str(r/'src/ps5_controller_speaker.cpp'),'-pthread','-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))

with tempfile.TemporaryDirectory(prefix='dkrr-speaker-files-') as tmp:
    out=pathlib.Path(tmp)/'reader'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src'),'-Wl,--wrap=open','-Wl,--wrap=read','-Wl,--wrap=fstat','-Wl,--wrap=close',str(r/'tests/ps5_pcm_reader.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out),tmp],check=True,timeout=10,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
