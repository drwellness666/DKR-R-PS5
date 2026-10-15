#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual native picker and marker with real ImGui, no console/ROM."""
import argparse,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
root=pathlib.Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_work',type=pathlib.Path,nargs='?')
parser.add_argument('--cache',type=pathlib.Path,action='append',default=[])
args=parser.parse_args();prepared=None
if args.game_work is None:
 prepared=tempfile.TemporaryDirectory(prefix='dkrr-play-inputs-')
 args.game_work=pathlib.Path(prepared.name)/'game'
 command=['python3',str(root/'tools/prepare_controller_test_sources.py'),str(args.game_work)]
 for cache in args.cache:command+=['--cache',str(cache)]
 subprocess.run(command,check=True)
source=args.game_work.resolve()/'source'
text=(source/'DKR-R/runtime-recomp/src/game/runtime_ui.cpp').read_text()
start=text.index('void DrawSidebarCurrentStripe()');end=text.index('\nbool SidebarButton(',start)
imgui=source/'rt64-game/src/contrib/imgui'
with tempfile.TemporaryDirectory(prefix='dkrr-play-nav-') as tmp:
 tmp=pathlib.Path(tmp);(tmp/'sidebar_marker.inc').write_text(text[start:end])
 out=tmp/'navigation'
 subprocess.run(['clang++','-std=c++20','-D__PROSPERO__','-O1','-Wall','-Wextra','-Werror',*imgui_warnings(),'-I',str(tmp),'-I',str(root/'src'),'-I',str(imgui),str(root/'tests/console_play_navigation.cpp'),*[str(imgui/file) for file in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)

if prepared is not None:prepared.cleanup()
