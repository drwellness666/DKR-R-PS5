#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,os,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('game',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-import-') as folder:
    t=Path(folder);out=t/'test'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DDKRR_NATIVE_IMPORT_ROOT="'+str(t/'data/dkrr')+'"','-I'+str(r/'src'),'-I'+str(a.game.resolve()/'source/DKR-R/runtime-recomp/src/game/mods'),str(r/'tests/ps5_import_control.cpp'),'-pthread','-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
