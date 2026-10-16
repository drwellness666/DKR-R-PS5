#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-ps5-page-') as tmp:
 t=pathlib.Path(tmp)
 if a.game_work is None:
  a.game_work=t/'inputs';cmd=['python3',str(r/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:cmd+=['--cache',str(cache)]
  subprocess.run(cmd,check=True)
 source=a.game_work/'source';text=(source/'DKR-R/runtime-recomp/src/game/runtime_controls_ui.inl').read_text();start=text.index('void DrawControlsPs5(');end=text.index('// ------------------------------------------------------------ shortcuts',start)
 (t/'ps5_controls_page.inc').write_text(text[start:end]);imgui=source/'rt64-game/src/contrib/imgui';out=t/'page'
 subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror',*imgui_warnings(),'-I'+str(t),'-I'+str(imgui),str(r/'tests/ps5_controls_page.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
