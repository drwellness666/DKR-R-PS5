// SPDX-License-Identifier: GPL-3.0-or-later
// Real PS5 page dispatch/callbacks with modeled devices and simple ImGui fields.
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
static bool modern=true;
namespace dkr::runtime::enhancements {int presentation_profile(){return 1;}bool modern_options_visible(int){return modern;}}
namespace dkr::runtime::platform {
static bool feedback=false,lights=false,speaker=false;static float volume=0.25F;
bool item_feedback_enabled(){return feedback;}void set_item_feedback_enabled(bool v){feedback=v;}
bool controller_lights_enabled(){return lights;}void set_controller_lights_enabled(bool v){lights=v;}
bool controller_speaker_enabled(){return speaker;}void set_controller_speaker_enabled(bool v){speaker=v;}
float controller_speaker_volume(){return volume;}void set_controller_speaker_volume(float v){volume=v;}
const char*controller_speaker_status(std::size_t){return "Experimental output";}
}
static constexpr bool kConsoleMenuLayout=true;static std::size_t g_selected_player=0;
static bool gyro=false;static unsigned motion_drawn=0,commits=0;
struct Field{ImGuiID id;ImVec2 min,max;};static std::vector<Field> fields;
static void add(){fields.push_back({ImGui::GetItemID(),ImGui::GetItemRectMin(),ImGui::GetItemRectMax()});}
static void PaddockGap(float f){ImGui::Dummy({0,f});}
static void ControlsScope(bool,float){}static void ControlsHeading(std::string_view s,float){ImGui::TextUnformatted(s.data(),s.data()+s.size());}
static void ControlsNote(std::string_view s,float){ImGui::TextWrapped("%.*s",int(s.size()),s.data());}
static bool SettingsCheck(const char*id,const char*label,bool*v,float){ImGui::PushID(id);bool c=ImGui::Checkbox(label,v);add();ImGui::PopID();return c;}
struct SettingsRangeValue{std::string text,unit;};
template<class Format>static bool SettingsRange(const char*id,const char*,int*v,int lo,int hi,float,Format){bool c=ImGui::SliderInt(id,v,lo,hi);add();return c;}
static void DrawControlsMotion(float w,bool,const std::function<void()>&commit){++motion_drawn;if(SettingsCheck("gyro","Enable gyro steering",&gyro,w))commit();}
#include "ps5_controls_page.inc"
static void frame(bool focus=false){
 ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1000,250});ImGui::Begin("Controls PS5",nullptr,ImGuiWindowFlags_NoTitleBar);fields.clear();
 if(focus)ImGui::SetKeyboardFocusHere();DrawControlsPs5(900,true,[]{++commits;});ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1200,900};io.DeltaTime=1.0F/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);frame(true);frame();assert(fields.size()==4&&GImGui->NavId==fields[0].id);
 press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);press(ImGuiKey_GamepadFaceDown);assert(dkr::runtime::platform::feedback&&commits==1);
 press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==fields[1].id);press(ImGuiKey_GamepadFaceDown);assert(dkr::runtime::platform::lights&&commits==2);
 press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(dkr::runtime::platform::speaker&&commits==3&&fields.size()==5);
 press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==fields[3].id);press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==fields[4].id);press(ImGuiKey_GamepadFaceDown);assert(gyro&&commits==4);
 for(std::size_t i=1;i<fields.size();++i)assert(fields[i].min.y>=fields[i-1].max.y);
 assert(GImGui->NavWindow->Scroll.y>0);modern=false;motion_drawn=0;frame();assert(motion_drawn==0&&fields.size()==4);
 ImGui::DestroyContext();puts("PASS actual PS5 page: D-pad order, Cross toggles light/trigger/speaker and gyro dispatch, volume insertion without focus loss, scrolling and Accurate-only gyro restriction");
}
