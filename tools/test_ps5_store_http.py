#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise native Store HTTPS flow with SDK fault doubles; no live requests."""
import os
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-store-http-') as folder:
    binary = Path(folder) / 'test'
    subprocess.run(['clang++', '-std=c++20', '-O1', '-g', '-fsanitize=address,undefined',
                    '-I'+str(root/'src'), str(root/'tests/ps5_store_http.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
print('PASS actual native Store HTTPS: resource cleanup at every SDK failure, TLS/redirect settings, GET/POST, headers/ranges, cancellation, bounded/truncated input and sink failure; no live PS5 claim')

with tempfile.TemporaryDirectory(prefix='dkrr-store-static-') as folder:
    binary = Path(folder) / 'test'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'src'),str(root/'tests/ps5_store_curl.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
