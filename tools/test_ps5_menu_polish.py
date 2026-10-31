#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real committed-save view and Store admission UI."""
from pathlib import Path
import argparse,os,subprocess,tempfile
from compiler_flags import imgui_warnings
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('game_work',type=Path);a=p.parse_args()
d=a.game_work.resolve()/'source/DKR-R/runtime-recomp/src/game'
actual=a.game_work.resolve()/'game-ps5/native-domain/src/game'
if actual.is_dir():d=actual
def function(text,name):
 start=text.index(name);start=text.rfind('\n',0,start)+1;at=text.index('{',start);end=at+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]+'\n'
with tempfile.TemporaryDirectory(prefix='dkrr-native-menu-') as tmp:
 t=Path(tmp)
 (t/'live_saves.inc').write_text(function((d/'runtime_saves_ui.inl').read_text(),'void DrawSavesLivePage('))
 (t/'store_status.inc').write_text(function((d/'runtime_online_ui.inl').read_text(),'bool DrawOnlineStoreStatus('))
 imgui=a.game_work.resolve()/'source/rt64-game/src/contrib/imgui'
 out=t/'test'
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-DDKR_RUNTIME_HAS_RT64=1','-fsanitize=address,undefined','-Wl,--wrap=lstat',*imgui_warnings(),'-I'+str(t),'-I'+str(r/'src'),'-I'+str(d),'-I'+str(imgui),str(r/'tests/ps5_menu_polish.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-pthread','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
