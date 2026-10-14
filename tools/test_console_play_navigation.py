#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual native picker and marker with real ImGui, no console/ROM."""
import pathlib,subprocess,sys,tempfile
if len(sys.argv)!=2:raise SystemExit('Usage: test_console_play_navigation.py ABSOLUTE_GAME_WORK')
root=pathlib.Path(__file__).resolve().parent.parent;source=pathlib.Path(sys.argv[1]).resolve()/'source'
text=(source/'DKR-R/runtime-recomp/src/game/runtime_ui.cpp').read_text()
start=text.index('void DrawSidebarCurrentStripe()');end=text.index('\nbool SidebarButton(',start)
imgui=source/'rt64-game/src/contrib/imgui'
with tempfile.TemporaryDirectory(prefix='dkrr-play-nav-') as tmp:
 tmp=pathlib.Path(tmp);(tmp/'sidebar_marker.inc').write_text(text[start:end])
 out=tmp/'navigation'
 subprocess.run(['clang++','-std=c++20','-D__PROSPERO__','-O1','-Wall','-Wextra','-Werror','-Wno-nontrivial-memcall','-Wno-uninitialized-const-pointer','-I',str(tmp),'-I',str(root/'src'),'-I',str(imgui),str(root/'tests/console_play_navigation.cpp'),*[str(imgui/file) for file in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
