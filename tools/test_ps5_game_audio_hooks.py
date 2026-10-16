#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual guest observers and native per-player sound-routing callbacks, no ROM."""
import argparse,hashlib,os,pathlib,subprocess,tarfile,tempfile
root=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-game-sounds-') as name:
 t=pathlib.Path(name)
 if a.game_work is None:
  a.game_work=t/'inputs';cmd=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:cmd+=['--cache',str(cache)]
  subprocess.run(cmd,check=True)
 game=a.game_work/'source/DKR-R/runtime-recomp/src/game';platform=(game/'runtime_platform.cpp').read_text();start=platform.index('namespace {\nbool NativeSoundContext(');end=platform.index('\n#endif',start);(t/'sound_callbacks.inc').write_text(platform[start:end])
 archive=root/'vendor/N64Recomp.tar.gz';expected=next(line.split('  ')[0] for line in (root/'vendor/SHA256SUMS').read_text().splitlines() if line.endswith('  N64Recomp.tar.gz'))
 assert hashlib.sha256(archive.read_bytes()).hexdigest()==expected
 with tarfile.open(archive) as src:
  entries=[m for m in src if m.isfile() and (m.name=='include/recomp.h' or m.name.endswith('/include/recomp.h'))];assert len(entries)==1;(t/'recomp.h').write_bytes(src.extractfile(entries[0]).read())
 binary=t/'sounds';subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'src'),'-I'+str(game),'-I'+str(t),str(root/'tests/ps5_game_audio_hooks.cpp'),str(root/'src/ps5_game_audio_hooks.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
