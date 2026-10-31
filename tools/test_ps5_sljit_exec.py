#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,os,subprocess,tempfile
from pathlib import Path
from stage_ps5_sljit import stage
p=argparse.ArgumentParser();p.add_argument('game',type=Path);p.add_argument('platform_headers',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1];source=a.game.resolve()/'source/N64ModernRuntime/N64Recomp/lib/sljit/sljit_src/sljitLir.c'
with tempfile.TemporaryDirectory(prefix='dkrr-native-exec-') as temporary:
 t=Path(temporary);staged=stage(source,t/'source');out=t/'test'
 subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-Wno-macro-redefined','-DDKRR_TEST_NATIVE_EXEC','-O1','-g','-fsanitize=address,undefined','-I'+str(r/'src'),'-I'+str(source.parent),'-I'+str(a.platform_headers.resolve()),str(staged),str(r/'tests/ps5_sljit_exec.c'),'-pthread','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,timeout=20,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1'))
