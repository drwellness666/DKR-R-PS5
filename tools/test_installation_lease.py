#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-install-lease-') as temp:
    p=pathlib.Path(temp);binary=p/'lease'
    subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),'-DDKRR_GAME_LEASE_PATH="'+str(p/'own.lock')+'"',str(root/'tests/installation_lease.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'),check=True)
