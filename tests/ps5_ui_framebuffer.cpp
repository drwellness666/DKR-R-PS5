// SPDX-License-Identifier: GPL-3.0-or-later
#include <imgui.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include "ps5_display_options.h"
static struct dkrr_ps5_display_status active{};
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status(){return active;}
void apply(){
#include "ps5_ui_framebuffer.inl"
}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;
 io.DisplaySize={1920,1080};io.DeltaTime=1.0F/60;io.Fonts->Build();
 for(const unsigned height:{1080U,1440U,2160U}){
  active.width=height==1080?1920:height==1440?2560:3840;active.height=height;
  io.DisplayFramebufferScale={1,1};ImGui::NewFrame();apply();
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::Begin("Full console menu",nullptr,ImGuiWindowFlags_NoDecoration);
  ImGui::TextUnformatted("Controller navigation stays in logical coordinates");ImGui::End();
  ImGui::Render();auto*data=ImGui::GetDrawData();assert(data&&data->TotalVtxCount>0);
  assert(data->DisplaySize.x==1920 && data->DisplaySize.y==1080);
  assert(std::lround(data->DisplaySize.x*data->FramebufferScale.x)==active.width);
  assert(std::lround(data->DisplaySize.y*data->FramebufferScale.y)==active.height);
  assert(io.FontGlobalScale==1.0F);
 }
 active={};io.DisplayFramebufferScale={1.25F,1.25F};apply();assert(io.DisplayFramebufferScale.x==1.25F);
 active.width=3840;active.height=2160;io.DisplaySize.x=0;apply();assert(io.DisplayFramebufferScale.x==1.25F);
 io.DisplaySize={NAN,1080};apply();assert(io.DisplayFramebufferScale.x==1.25F);
 io.DisplaySize={1,1};apply();assert(io.DisplayFramebufferScale.x==1.25F);
 ImGui::DestroyContext();std::puts("PASS actual ImGui frame: full 1080p/1440p/4K pixel extent, stable logical layout/font scale, absent/invalid-state refusal");
}
