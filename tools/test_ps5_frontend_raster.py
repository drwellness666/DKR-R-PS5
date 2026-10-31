#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the actual software frontend adapter on offscreen SDL surfaces."""
import argparse,os,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path);a=p.parse_args()
g=a.game_work.resolve()/'source/DKR-R/runtime-recomp/src/game';imgui=a.game_work.resolve()/'source/rt64-game/src/contrib/imgui'
with tempfile.TemporaryDirectory(prefix='dkrr-frontend-raster-') as temporary:
    t=pathlib.Path(temporary);text=(g/'runtime_ui.cpp').read_text();start=text.index('struct LauncherDrawProfileRange {');end=text.index('void DrawLinuxSoftwarePanelUnderlay(',start)
    (t/'frontend.inc').write_text(text[start:end])
    flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2'],text=True).split()
    output=t/'raster'
    subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',*imgui_warnings(),'-fsanitize=address,undefined','-I'+str(t),'-I'+str(g),'-I'+str(imgui),str(r/'tests/ps5_frontend_raster.cpp'),str(imgui/'backends/imgui_impl_sdlrenderer2.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],*flags,'-o',str(output)],check=True)
    subprocess.run([str(output)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
