// SPDX-License-Identifier: GPL-3.0-or-later
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "ps5_menu_layout.hpp"
#include "ps5_menu_sections.inl"
constexpr auto kSidebarOrder=dkrr::ps5_menu::selection_order;
#include "sidebar_step.inc"
static int current=0,changes=0;
static float width=640;
static bool disabled=false;
static ImGuiID selector=0,after=0;
static void frame(bool focus=false) {
 ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1100,600});
 ImGui::Begin("PS5 sections",nullptr,ImGuiWindowFlags_NoTitleBar);
 ImGui::PushID("graphics");selector=ImGui::GetID("##section");ImGui::PopID();
 if(focus)ImGui::SetKeyboardFocusHere();
 ImGui::BeginDisabled(disabled);
 int picked=DrawPs5SectionSelector("graphics",{"Display","HUD & Detail","Water"},current,width);
 if(picked>=0&&picked!=current){current=picked;++changes;}
 ImGui::EndDisabled();
 if(GImGui->OpenPopupStack.Size==0)assert(ImGui::GetItemRectMax().x-ImGui::GetItemRectMin().x<=width+0.5F);
 ImGui::Button("Next setting");after=ImGui::GetItemID();
 ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key) {
 auto& io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();
}
void run_ps5_update_ui_tests();
int main() {
 assert(StepSidebarSelection(0,1)==3&&StepSidebarSelection(3,-1)==0);
 int at=0;std::array<bool,11> seen{};
 for(int i=0;i<11;++i){assert(at>=0&&at<11&&!seen[at]);seen[at]=true;int next=StepSidebarSelection(at,1);assert(StepSidebarSelection(next,-1)==at);at=next;}
 assert(at==0);
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1200,800};io.DeltaTime=1.0F/60;
 io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
 for(float tested_width:{280.0F,640.0F,1000.0F}) {
  width=tested_width;current=0;changes=0;frame(true);frame();assert(GImGui->NavId==selector);
  press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==after&&current==0&&changes==0);
  frame(true);frame();press(ImGuiKey_GamepadFaceDown);assert(GImGui->OpenPopupStack.Size>0);
  press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(current==1&&changes==1&&GImGui->OpenPopupStack.Size==0);
  frame(true);frame();press(ImGuiKey_GamepadFaceDown);press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceRight);
  assert(current==1&&changes==1&&GImGui->OpenPopupStack.Size==0); // Circle cancels.
  disabled=true;frame(true);frame();press(ImGuiKey_GamepadFaceDown);assert(current==1&&changes==1);disabled=false;
 }
 ImGui::DestroyContext();run_ps5_update_ui_tests();puts("PASS actual PS5 section selector: bounded narrow/wide geometry, D-pad order, Cross selection, Circle cancellation, disabled state; native visual and shoulder page order agree");
}
