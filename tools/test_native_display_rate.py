#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the actual native renderer rate getter, with no GPU/ROM/console."""
import argparse,os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_work',type=pathlib.Path,nargs='?')
p.add_argument('--cache',type=pathlib.Path,action='append',default=[])
a=p.parse_args();prepared=None
if a.game_work is None:
    prepared=tempfile.TemporaryDirectory(prefix='dkrr-rate-inputs-');a.game_work=pathlib.Path(prepared.name)/'game'
    command=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
    for cache in a.cache:command+=['--cache',str(cache)]
    subprocess.run(command,check=True)
source=(a.game_work/'source/DKR-R/runtime-recomp/src/game/rt64_renderer.cpp').read_text()
start=source.index('int DetectDisplayRate() {');end=source.index('\nvoid ApplyConfig(',start)
with tempfile.TemporaryDirectory(prefix='dkrr-native-display-rate-') as folder:
    folder=pathlib.Path(folder);(folder/'detect_display_rate.inc').write_text(source[start:end]);out=folder/'test'
    subprocess.run(['clang++','-D__PROSPERO__','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-include','initializer_list','-I',str(root/'src'),
                    '-I',str(folder),str(root/'tests/native_display_rate.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'),check=True)
if prepared is not None:prepared.cleanup()
