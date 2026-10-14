// SPDX-License-Identifier: GPL-3.0-or-later
// Uses the actual patched SettingsColumns definition, with real Dear ImGui navigation.
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include "settings_columns.inc"
static std::array<bool,8> values{};
static std::array<ImGuiID,8> ids{};
static void check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s nav=%u\n",message,GImGui->NavId);std::exit(1);}}
static void frame(bool focus=false){
 ImGui::NewFrame();
 ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1100,300});
 ImGui::Begin("Console settings",nullptr,ImGuiWindowFlags_NoTitleBar);
 SettingsColumns grid(1000,false);
 for(int n=0;n<8;++n){grid.Begin();ImGui::PushID(n);
  if(focus&&n==0)ImGui::SetKeyboardFocusHere();
  if(n==3)ImGui::BeginDisabled();
  ImGui::Checkbox("Setting",&values[n]);ids[n]=ImGui::GetItemID();
  if(n==3)ImGui::EndDisabled();
  // Different caption heights reproduce the diagonal focus choices in a desktop grid.
  if(n%2==0)ImGui::Dummy({100,42});
  ImGui::PopID();grid.End();}
 grid.Finish();ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.DisplaySize={1200,800};io.DeltaTime=1.0f/60;
 io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;
 io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
 frame(true);frame();check(GImGui->NavId==ids[0],"initial focus");
 for(int next:{1,2,4,5,6,7}){press(ImGuiKey_GamepadDpadDown);check(GImGui->NavId==ids[next],"Down follows field order and skips disabled items");}
 check(GImGui->NavWindow->Scroll.y>0,"offscreen fields scroll into view");
 press(ImGuiKey_GamepadFaceDown);check(values[7],"Cross activates selected checkbox");
 for(int n=0;n<7;++n)check(!values[n],"Cross does not change another checkbox");
 press(ImGuiKey_GamepadDpadUp);check(GImGui->NavId==ids[6],"Up reverses selection");
 ImGui::DestroyContext();std::puts("PASS: console D-pad order, disabled items, scrolling, Cross activation and reverse navigation");
}
