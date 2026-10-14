#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the real PS5 interface font loader on the host using only Selawik."""
import argparse,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_work',type=pathlib.Path);a=p.parse_args()
s=a.game_work.resolve()/'source';game=s/'DKR-R/runtime-recomp/src/game';imgui=s/'rt64-game/src/contrib/imgui'
with tempfile.TemporaryDirectory(prefix='dkrr-ofl-fonts-') as temp:
 temp=pathlib.Path(temp);font=temp/'font.o';out=temp/'fonts'
 subprocess.run(['clang','-std=c11','-O1','-I',str(game/'generated'),'-c',str(game/'generated/selawik_font.c'),'-o',str(font)],check=True)
 subprocess.run(['clang++','-D__PROSPERO__','-std=c++20','-O1','-Wno-nontrivial-memcall','-I',str(root/'src'),'-I',str(game),'-I',str(imgui),str(root/'tests/console_fonts.cpp'),str(font),*[str(imgui/name) for name in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
