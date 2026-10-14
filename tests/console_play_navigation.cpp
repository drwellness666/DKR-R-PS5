// SPDX-License-Identifier: GPL-3.0-or-later
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "ps5_revision_selection.h"
static struct dkrr_ps5_revision_state state{77,77,3,3};
static unsigned calls=0;
extern "C" struct dkrr_ps5_revision_state dkrr_ps5_revision_state(){return state;}
extern "C" int dkrr_ps5_choose_revision(unsigned revision){++calls;state.preferred=revision;return 0;}
static void check(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
#include "sidebar_marker.inc"
static ImGuiID original,revision,resume;
static bool resumed=false;
static void frame(bool focus=false){
 ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1000,650});
 ImGui::Begin("Play",nullptr,ImGuiWindowFlags_NoTitleBar);
 ImGui::Button("PLAY",{200,46});
 const auto vertices=ImGui::GetWindowDrawList()->VtxBuffer.Size;
 const auto commands=ImGui::GetWindowDrawList()->CmdBuffer.Size;
 DrawSidebarCurrentStripe();
 check(ImGui::GetWindowDrawList()->VtxBuffer.Size==vertices&&ImGui::GetWindowDrawList()->CmdBuffer.Size==commands,"native sidebar adds no stripe geometry or scissor command");
 if(focus)ImGui::SetKeyboardFocusHere();
 original=ImGui::GetID("US 1.0 - original");revision=ImGui::GetID("US 1.1 - Rev A");
 #include "ps5_revision_ui.inl"
 if(ImGui::Button("RETURN TO RACE"))resumed=true;resume=ImGui::GetItemID();
 ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.DisplaySize={1200,800};io.DeltaTime=1.0f/60;
 io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
 frame(true);frame();check(GImGui->NavId==original,"initial focus is US 1.0");
 press(ImGuiKey_GamepadDpadDown);check(GImGui->NavId==revision,"Down selects next version");
 press(ImGuiKey_GamepadFaceDown);check(calls==1&&state.preferred==80&&state.active==77,"Cross chooses next launch only, not current game");
 press(ImGuiKey_GamepadDpadDown);check(GImGui->NavId==resume,"Down reaches return button");
 press(ImGuiKey_GamepadFaceDown);check(resumed,"Cross resumes through return button");
 state.available_mask=1;state.preferred=77;io.DeltaTime=1.1f;frame(true);frame();io.DeltaTime=1.0f/60;
 check(GImGui->NavId==original,"US 1.0 retains focus when Rev A ROM unavailable");
 press(ImGuiKey_GamepadDpadDown);check(GImGui->NavId==resume,"unavailable ROM choice is skipped");
 check(calls==1,"navigation alone never changes version");
 ImGui::DestroyContext();std::puts("PASS actual Play version UI, next-launch ownership, disabled-ROM navigation, resume and no sidebar stripe/scissor");
}
