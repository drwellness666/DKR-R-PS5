#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Crash-resilient writes through all three actual native preference adapters."""
import os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-preferences-') as name:
    work=pathlib.Path(name);data=work/'data';output=work/'preferences'
    subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined','-pthread','-Wl,--wrap=fsync',
        '-DDKR_PS5_HIGH_REFRESH_BUILD=1',f'-DDKR_DISPLAY_DATA_ROOT="{data}"',
        f'-DDKR_REVISION_DATA_ROOT="{data}"','-I'+str(root/'src'),
        str(root/'tests/ps5_preferences.cpp'),
        *[str(root/'src'/f'ps5_{name}.cpp') for name in ('display_options','hdr_options','revision_selection')],
        '-o',str(output)],check=True)
    subprocess.run([str(output)],check=True,timeout=30,
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
