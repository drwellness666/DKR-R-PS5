// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_internet_update.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
namespace {
dkrr::updates::View update_view{};
bool automatic=false,save_ok=false;
unsigned saved=0,checked=0,installed=0,cancelled=0;
float width=640;
std::vector<std::string> messages;
std::string focus_button;
bool focus_preference=false,install_disabled=false,check_disabled=false;
constexpr ImVec4 kSetWarm{1,0.8F,0.4F,1},kSetMuted{0.7F,0.8F,0.9F,1};
template<class Fn>void DetailCard(float value,ImVec2,Fn fn){fn(value);}
void DetailHeading(const char* text,float,ImVec4){ImGui::TextUnformatted(text);}
void DetailText(const char*text,float inner,ImVec4=kSetMuted){messages.emplace_back(text);ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+inner);ImGui::TextUnformatted(text);ImGui::PopTextWrapPos();}
void PaddockGap(float value){ImGui::Dummy({0,value});}
bool DetailRaceButton(const char*text,float inner){
 if(focus_button==text)ImGui::SetKeyboardFocusHere();
 bool pressed=ImGui::Button(text,{inner,30});
 const bool disabled=(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled)!=0;
 if(std::string(text)=="CHECK FOR UPDATES")check_disabled=disabled;
 if(std::string(text)=="INSTALL UPDATE AND CLOSE DKR-R")install_disabled=disabled;
 return pressed;
}
void frame(){
 messages.clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1120,850});
 ImGui::Begin("Update",nullptr,ImGuiWindowFlags_NoTitleBar);
 if(focus_preference)ImGui::SetKeyboardFocusHere();
 #include "ps5_update_ui.inl"
 ImGui::End();ImGui::Render();
}
bool has(const char*needle){return std::any_of(messages.begin(),messages.end(),[&](const auto&s){return s.find(needle)!=std::string::npos;});}
void press(ImGuiKey key=ImGuiKey_GamepadFaceDown){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
}
namespace dkrr::updates {
View view(){return update_view;}
bool automatic_check(){return automatic;}
bool set_automatic_check(bool value){++saved;if(save_ok)automatic=value;return save_ok;}
void check(){++checked;}
void install(){++installed;}
void cancel(){++cancelled;}
}
void run_ps5_update_ui_tests(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.DisplaySize={1200,900};io.DeltaTime=1.0F/60;io.IniFilename=nullptr;
 io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char*p;int x,y;io.Fonts->GetTexDataAsRGBA32(&p,&x,&y);
 focus_preference=true;frame();frame();focus_preference=false;
 // Keyboard-focus setup alone leaves ImGui in its initial mouse-highlight mode.
 // Enter gamepad navigation through the same D-pad events used by the console.
 press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);
 assert(GImGui->NavId==ImHashStr("Check for updates at launch",0,ImHashStr("Update")));
 assert(!automatic&&!has("once each launch"));press();assert(saved==1&&!automatic);
 for(int i=0;i<20;++i){frame();assert(has("update preference"));}
 assert(saved==1);save_ok=true;press();assert(saved==2&&automatic&&!has("update preference"));
 for(float tested_width:{280.0F,640.0F,1000.0F}){
  width=tested_width;update_view={};update_view.state=dkrr::updates::State::Available;update_view.space_required=10*1024*1024;update_view.space_known=true;update_view.space_available=update_view.space_required-1;
  frame();assert(install_disabled&&has("Not enough free space"));
  update_view.space_available=update_view.space_required;frame();assert(!install_disabled&&!has("Not enough free space"));
  update_view.space_known=false;frame();assert(!install_disabled&&has("could not be read"));
 }
 width=640;focus_button="INSTALL UPDATE AND CLOSE DKR-R";frame();frame();focus_button.clear();press();assert(installed==1);
 update_view.state=dkrr::updates::State::Checking;frame();assert(check_disabled);
 update_view.state=dkrr::updates::State::Downloading;update_view.bytes=100;update_view.received=25;focus_button="CANCEL DOWNLOAD";frame();frame();focus_button.clear();press();assert(cancelled==1&&check_disabled);
 update_view.state=dkrr::updates::State::Idle;focus_button="CHECK FOR UPDATES";frame();frame();focus_button.clear();press();assert(checked==1);
 ImGui::DestroyContext();puts("PASS actual update card with real ImGui: persistent save failure, successful retry, no idle writes, honest default, storage-disabled install/reason and exact boundary, check/install/cancel gamepad dispatch");
}
