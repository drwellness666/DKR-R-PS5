#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test the actual texture-import scheduling/shutdown with host I/O doubles."""
import argparse,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-texture-worker-') as folder:
 t=pathlib.Path(folder)
 if a.game_work is None:
  a.game_work=t/'inputs';command=['python3',str(r/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:command+=['--cache',str(cache)]
  subprocess.run(command,check=True)
 text=(a.game_work/'source/DKR-R/runtime-recomp/src/game/runtime_ui.cpp').read_text()
 state=text[text.index('struct TextureImportState {'):text.index('char g_texture_pack_search[160]')]
 start=text[text.index('bool StartTexturePackImport('):text.index('// A native file or folder picker is modal')]
 at=text.index('extern "C" void dkrr_ps5_shutdown_dialogs() {');shutdown=text[at:text.index('\n#endif',at)]
 if state.count('std::jthread')!=1 or start.count('std::jthread')!=1:raise RuntimeError('Texture thread test seam changed')
 (t/'texture_state.inc').write_text(state.replace('std::jthread','FixtureThread'))
 (t/'texture_start.inc').write_text(start.replace('std::jthread','FixtureThread'))
 (t/'texture_shutdown.inc').write_text(shutdown)
 binary=t/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(t),str(r/'tests/ps5_texture_import_worker.cpp'),'-o',str(binary)],check=True)
 for mode in ('start','shutdown','commit'):subprocess.run([str(binary),mode],check=True,timeout=20)
