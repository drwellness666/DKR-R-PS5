#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise held Square against the linked menu library and native routing."""
from pathlib import Path
import argparse,os,subprocess,tempfile
from compiler_flags import imgui_warnings
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game',type=Path);a=p.parse_args()
g=a.game.resolve();ui=g/'source/rt64-game/src/contrib/imgui'
platform=(g/'game-ps5/native-domain/src/game/runtime_platform.cpp').read_text()
needle='    button(ImGuiKey_GamepadFaceLeft, SDL_CONTROLLER_BUTTON_X);'
at=platform.index(needle);begin=platform.rfind('#if defined(__PROSPERO__)',0,at);end=platform.index('#endif',at)+len('#endif')
fragment=platform[begin:end];assert 'io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,false);' in fragment
source='''#include "imgui.h"
#include "imgui_internal.h"
#include <cassert>
static constexpr int SDL_CONTROLLER_BUTTON_X=0;
int main(){IMGUI_CHECKVERSION();ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1280,720};io.DeltaTime=1.F/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
for(int fixed=0;fixed<2;++fixed){bool windowing=false;for(int frame=0;frame<180;++frame){auto button=[&](ImGuiKey key,int){io.AddKeyEvent(key,frame>5&&frame<150);};
#if FIXED
@FRAGMENT@
#else
button(ImGuiKey_GamepadFaceLeft,SDL_CONTROLLER_BUTTON_X);
#endif
io.AddKeyEvent(ImGuiKey_GamepadFaceDown,frame==170);ImGui::NewFrame();ImGui::Begin("DKR-R");ImGui::Button("Return to race");ImGui::End();windowing|=GImGui->NavWindowingTarget!=nullptr;ImGui::Render();}assert(windowing==!bool(FIXED));}ImGui::DestroyContext();}
'''.replace('@FRAGMENT@',fragment)
with tempfile.TemporaryDirectory(prefix='dkrr-windowing-') as folder:
 t=Path(folder);(t/'test.cpp').write_text(source)
 for fixed in (0,1):
  out=t/f'test-{fixed}'
  subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-Wno-unused-variable',*imgui_warnings(),'-fsanitize=address,undefined',f'-DFIXED={fixed}','-D__PROSPERO__','-I'+str(ui),str(t/'test.cpp'),*[str(ui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
assert platform.count('io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,false);')==2
print('PASS actual ImGui: held Square starts desktop window switching before the fix; both native menu routes suppress it without changing physical game bindings')
