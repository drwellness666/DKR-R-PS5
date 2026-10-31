#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,os,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path);a=p.parse_args()
imgui=a.game_work.resolve()/'source/rt64-game/src/contrib/imgui'
with tempfile.TemporaryDirectory(prefix='dkrr-pointer-') as temporary:
    output=pathlib.Path(temporary)/'pointer'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',*imgui_warnings(),'-fsanitize=address,undefined','-I'+str(r/'src'),'-I'+str(imgui),str(r/'tests/ps5_menu_pointer.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(output)],check=True)
    subprocess.run([str(output)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
