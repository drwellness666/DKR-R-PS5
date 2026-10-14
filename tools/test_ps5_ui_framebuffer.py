#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test the actual PS5 scale hook with pinned ImGui; no ROM or console."""
import argparse,os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path,nargs='?',default=root/'work/ui-scale-tests');a=p.parse_args()
subprocess.run(['python3',str(root/'tools/prepare_game_sources.py'),str(a.game_work)],check=True)
imgui=a.game_work/'source/rt64-game/src/contrib/imgui'
warnings=[]
for warning in ('-Wno-nontrivial-memcall','-Wno-nontrivial-memaccess'):
 probe=subprocess.run(['clang++','-Werror','-Wunknown-warning-option',warning,'-x','c++','-fsyntax-only','-'],input='',text=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 if probe.returncode==0:warnings.append(warning)
with tempfile.TemporaryDirectory(prefix='dkrr-ui-scale-') as temp:
 out=pathlib.Path(temp)/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g',*warnings,'-fsanitize=address,undefined','-I'+str(root/'src'),'-I'+str(imgui),str(root/'tests/ps5_ui_framebuffer.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
