// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_controller_diagram.hpp"
#include "runtime_input.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <vector>
namespace dkr::runtime::enhancements {bool modern_presentation_enabled(){return false;}}
static unsigned g_selected_player=0,commits=0;static int captured=-1;static bool secondary=false;
struct ControlsPageState{bool mapping_open=false;std::optional<int> undo;std::string undo_message;};
static int TakeControlsSnapshot(){return 1;}
enum class CaptureDevice{Controller};
static void BeginControllerCapture(int action,CaptureDevice,bool alternate){captured=action;secondary=alternate;}
static std::string FriendlyBinding(std::string value){return value;}
static void PaddockGap(float n){ImGui::Dummy({0,n});}
struct SettingsChoice{std::string label;SettingsChoice(const char*text):label(text){}};
struct Field{ImGuiID id;ImVec2 min,max;std::string label;};static std::vector<Field> fields;
static void field(const std::string& name){fields.push_back({ImGui::GetItemID(),ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),name});}
static bool SettingsButton(const char*label,float width){bool result=ImGui::Button(label,{width,44});field(label);return result;}
static bool ControlsDisclosure(const char*,const char*label,bool&open,float width){bool result=SettingsButton(label,width);if(result)open=!open;return open;}
static bool SettingsDropdown(const char*id,const char*,int*selection,const std::vector<SettingsChoice>&choices,float width){
 ImGui::SetNextItemWidth(width);bool changed=false;if(ImGui::BeginCombo(id,choices[*selection].label.c_str())){for(unsigned i=0;i<choices.size();++i)if(ImGui::Selectable(choices[i].label.c_str(),*selection==int(i))){*selection=int(i);changed=true;}ImGui::EndCombo();}field(id);return changed;
}
#include "ps5_controller_diagram.inl"
#include "compact_bindings.inc"
static ControlsPageState state;
static void frame(bool focus=false){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1000,700});ImGui::Begin("Mapping",nullptr,ImGuiWindowFlags_NoTitleBar);fields.clear();if(focus)ImGui::SetKeyboardFocusHere();DrawPs5CompactBindings(state,900,[]{++commits;});ImGui::End();ImGui::Render();}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
static dkrr::ps5_pad::Bindings bindings(unsigned player){dkrr::ps5_pad::Bindings b;for(unsigned i=0;i<b.primary.size();++i){auto a=static_cast<dkr::runtime::input::Action>(i);b.primary[i]=dkr::runtime::input::controller_binding(player,a);b.alternate[i]=dkr::runtime::input::secondary_controller_binding(player,a);}return b;}
int main(){using namespace dkr::runtime::input;reset_defaults();ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1400,1000};io.DeltaTime=1.0F/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
 auto b=bindings(0);assert(dkrr::ps5_pad::assigned(b,0)=="Accelerate");assert(dkrr::ps5_pad::assigned(b,1009)=="Item / horn");assert(dkrr::ps5_pad::stick_label(b,false)=="L stick: steering");assert(dkrr::ps5_pad::assigned(b,4)=="Settings");
 set_controller_binding(1,Action::A,3);b=bindings(1);assert(dkrr::ps5_pad::selected(b,3,4));assert(dkrr::ps5_pad::assigned(b,3).find("Accelerate")!=std::string::npos);assert(dkrr::ps5_pad::assigned(bindings(0),0)=="Accelerate");
 frame(true);frame();assert(fields.size()==3);assert(fields.back().max.y-fields.front().min.y<180);press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==fields[1].id);press(ImGuiKey_GamepadFaceDown);assert(captured==4&&!secondary&&commits==0);
 press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(state.mapping_open&&fields.size()==5);press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(secondary&&captured==4);
 set_controller_binding(0,Action::A,3);press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(controller_preset(0)==ControllerPreset::Classic&&commits==1&&state.undo);
 g_selected_player=1;frame();assert(fields.size()==6);assert(fields[1].label.find("y")!=std::string::npos); // actual custom player binding
 for(float width:{280.0F,420.0F,700.0F,1000.0F}){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1300,800});ImGui::Begin("Bounds",nullptr,ImGuiWindowFlags_NoTitleBar);const auto at=ImGui::GetCursorScreenPos();auto*draw=ImGui::GetWindowDrawList();int before=draw->VtxBuffer.Size;DrawPs5ControllerDiagram(bindings(1),4,width);for(int i=before;i<draw->VtxBuffer.Size;++i){auto pos=draw->VtxBuffer[i].pos;assert(std::isfinite(pos.x)&&std::isfinite(pos.y));assert(pos.x>=at.x-1&&pos.x<=at.x+width+1);assert(pos.y>=at.y-1&&pos.y<=at.y+225);}ImGui::End();ImGui::Render();}
 ImGui::DestroyContext();puts("PASS actual compact mapping UI: live per-player bindings, Classic reset/undo, Cross capture, alternate disclosure, compact navigation and finite in-bounds vector geometry at four widths");}
