#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual compact mapping UI with live input bindings and gamepad navigation."""
import argparse,os,pathlib,shlex,subprocess,tempfile
from compiler_flags import imgui_warnings
root=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-pad-diagram-') as name:
 t=pathlib.Path(name)
 if a.game_work is None:
  a.game_work=t/'inputs';cmd=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:cmd+=['--cache',str(cache)]
  subprocess.run(cmd,check=True)
 source=a.game_work/'source';game=source/'DKR-R/runtime-recomp/src/game';text=(game/'runtime_controls_ui.inl').read_text();start=text.index('void DrawPs5CompactBindings(');end=text.index('\n#endif',start)
 (t/'compact_bindings.inc').write_text(text[start:end]);imgui=source/'rt64-game/src/contrib/imgui';sdl=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','sdl2'],text=True));out=t/'diagram'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',*imgui_warnings(),'-fsanitize=address,undefined','-DDKR_RUNTIME_HAS_RT64=1','-I'+str(root/'src'),'-I'+str(t),'-I'+str(imgui),'-I'+str(game),str(root/'tests/ps5_controller_diagram.cpp'),str(game/'runtime_input.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],*sdl,'-pthread','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
