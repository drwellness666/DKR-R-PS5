#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise ImGui internal navigation with both menu input providers present."""
from pathlib import Path
import argparse,os,subprocess,tempfile
from compiler_flags import imgui_warnings
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game',type=Path);a=p.parse_args()
g=a.game.resolve();ui=g/'source/rt64-game/src/contrib/imgui'
source=r'''#include "ps5_menu_window_policy.hpp"
#include <cassert>
#include <cstdio>
#include <algorithm>
int main(){IMGUI_CHECKVERSION();ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1280,720};io.DeltaTime=1.F/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
#if FIXED
ImGuiContextHook hook{};hook.Type=ImGuiContextHookType_NewFramePre;hook.Callback=[](ImGuiContext*,ImGuiContextHook*){dkrr::menu_window_policy::before_navigation();};ImGui::AddContextHook(GImGui,&hook);hook.Type=ImGuiContextHookType_NewFramePost;hook.Callback=[](ImGuiContext*,ImGuiContextHook*){dkrr::menu_window_policy::after_navigation();};ImGui::AddContextHook(GImGui,&hook);
#endif
bool windowing=false;unsigned square_actions=0,cross_actions=0;int peak_queue=0;
for(int frame=0;frame<5000;++frame){const bool square=frame>5&&frame<150;
 // ImGui_ImplSDL2_UpdateGamepads submits physical Square before the app's
 // NewFramePre input hook. A false native event conflicts with that state.
 io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,square);
 io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,FIXED?square:false);
 io.AddKeyEvent(ImGuiKey_GamepadDpadDown,frame==160);
 io.AddKeyEvent(ImGuiKey_GamepadFaceDown,frame==170);
 ImGui::NewFrame();peak_queue=std::max(peak_queue,GImGui->InputEventsQueue.Size);
 ImGui::Begin("DKR-R");if(frame<3)ImGui::SetKeyboardFocusHere();if(ImGui::Button("Return to race"))++cross_actions;
 if(ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft,false))++square_actions;
 ImGui::End();windowing|=GImGui->NavWindowingTarget!=nullptr;ImGui::Render();}
printf("FIXED=%d windowing=%d queued_peak=%d square_actions=%u cross_actions=%u\n",FIXED,windowing,peak_queue,square_actions,cross_actions);fflush(stdout);
#if FIXED
assert(!windowing&&square_actions==1&&cross_actions==1&&peak_queue<=2);
#else
assert(windowing&&peak_queue>4);
#endif
ImGui::DestroyContext();}
'''
with tempfile.TemporaryDirectory(prefix='dkrr-windowing-') as folder:
 t=Path(folder);(t/'test.cpp').write_text(source)
 for fixed in (0,1):
  out=t/f'test-{fixed}'
  subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror',*imgui_warnings(),'-fsanitize=address,undefined',f'-DFIXED={fixed}','-I'+str(ui),'-I'+str(r/'src'),str(t/'test.cpp'),*[str(ui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
domain=g/'game-ps5/native-domain/src/game'
if domain.is_dir():
 platform=(domain/'runtime_platform.cpp').read_text();text=(domain/'runtime_ui.cpp').read_text()
 assert 'io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,false)' not in platform
 assert 'static_cast<std::size_t>(StandardButton::West)' in platform and 'button(ImGuiKey_GamepadFaceLeft, SDL_CONTROLLER_BUTTON_X)' in platform
else:
 # ROM-free CI has the pinned menu library and authored patch, rather than
 # compiled guest/runtime objects. Native linking separately checks its real
 # compiler dependency on this policy header.
 text='\n'.join(line[1:] for line in (r/'patches/dkrr-109-native-final.patch').read_text().splitlines() if line.startswith(('+',' ')))
for contract in ('ImGuiContextHookType_NewFramePre','ImGuiContextHookType_NewFramePost','dkrr::menu_window_policy::before_navigation()','dkrr::menu_window_policy::after_navigation()'):assert contract in text
print('PASS actual ImGui over5000 frames: conflicting Square events reproduce window switching and a growing queue; scoped native navigation policy removes both while preserving Square widget actions and Cross activation')
