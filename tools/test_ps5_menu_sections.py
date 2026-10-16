#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real ImGui section selection plus the actual patched sidebar stepping code."""
import argparse,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game_work',nargs='?',type=pathlib.Path);p.add_argument('--cache',action='append',type=pathlib.Path,default=[]);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-menu-sections-') as temporary:
 w=pathlib.Path(temporary)
 if a.game_work is None:
  a.game_work=w/'inputs';cmd=['python3',str(r/'tools/prepare_controller_test_sources.py'),str(a.game_work)]
  for cache in a.cache:cmd+=['--cache',str(cache)]
  subprocess.run(cmd,check=True)
 source=a.game_work/'source';text=(source/'DKR-R/runtime-recomp/src/game/runtime_ui.cpp').read_text();start=text.index('int StepSidebarSelection(');end=text.index('\n}',start)+2
 (w/'sidebar_step.inc').write_text(text[start:end]);imgui=source/'rt64-game/src/contrib/imgui'
 subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror',*imgui_warnings(),'-I'+str(w),'-I'+str(r/'src'),'-I'+str(imgui),str(r/'tests/ps5_menu_sections.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(w/'test')],check=True)
 subprocess.run([str(w/'test')],check=True)
