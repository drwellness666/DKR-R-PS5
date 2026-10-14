// SPDX-License-Identifier: GPL-3.0-or-later
// Real preset and binding-row functions, with Dear ImGui gamepad navigation.
// Font styling/device discovery are replaced by fixture presentation helpers.
#include "runtime_input.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace dkr::runtime::enhancements { bool modern_presentation_enabled() { return false; } }
constexpr bool kConsoleMenuLayout = true;
constexpr unsigned kSetText=0xFFFFFF, kSetMuted=0xAAAAAA, kCtlLine=0x334455;
static std::size_t g_selected_player=0;
struct PaddockType { float line=16.0F; };
static PaddockType PaddockReading(float n, bool, float) { return {n}; }
static ImU32 PaddockRgb(unsigned n) { return n | 0xFF000000; }
static ImU32 PaddockCol(unsigned n, unsigned a=255) { return n | (a << 24); }
static void PaddockGap(float n) { ImGui::Dummy({0,n}); }
static void PaddockText(PaddockType, ImU32, std::string_view text, float) { ImGui::TextUnformatted(text.data(),text.data()+text.size()); }
static void PaddockDrawRun(ImDrawList* d, PaddockType, ImVec2 p, ImU32 c, const char* a,const char* b) { d->AddText(p,c,a,b); }
static void ControlsScope(bool, float) { ImGui::TextUnformatted("Player 1"); }
static void ControlsHeading(std::string_view t, float) { ImGui::TextUnformatted(t.data(),t.data()+t.size()); }
static void ControlsNote(std::string_view t, float) { ImGui::TextWrapped("%.*s",static_cast<int>(t.size()),t.data()); }
enum class SettingsButtonTone { Unbound, Outline, Plain };
struct Field { ImGuiID id; ImVec2 min,max; std::string name; };
static std::vector<Field> fields;
static bool SettingsButton(const char* text,float width=700,bool disabled=false,
    SettingsButtonTone=SettingsButtonTone::Plain,bool=false,const char* =nullptr,float=0) {
    ImGui::BeginDisabled(disabled);
    bool clicked=ImGui::Button(text,{width,44});
    fields.push_back({ImGui::GetItemID(),ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),text});
    ImGui::EndDisabled();
    return clicked;
}
static bool ControlsDisclosure(const char*,const char*,bool&,float) { return false; }
enum class CaptureDevice { Keyboard, Controller };
static int g_capture_action=-1;
static bool g_capture_secondary_controller=false,g_capture_popup_pending=false,g_capture_finished=false;
static CaptureDevice g_capture_device=CaptureDevice::Controller;
#include "controls_state.inc"
#include "controller_capture_begin.inc"
#include "friendly_binding.inc"
#include "controls_preset.inc"
#include "controls_bindings.inc"
static ControlsPageState& page=g_controls_page;
static int saved=0;
static void frame(bool focus=false) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1000,600});
    ImGui::Begin("PS5 controls",nullptr,ImGuiWindowFlags_NoTitleBar);
    fields.clear();
    if(focus)ImGui::SetKeyboardFocusHere();
    DrawControlsBindings(page,900,[]{++saved;});
    ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key) {
    auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();
    io.AddKeyEvent(key,false);frame();frame();
}
int main() {
    using namespace dkr::runtime::input;
    reset_defaults();
    page.undo=TakeControlsSnapshot();
    SelectControlsPlayer(page,0);
    assert(page.undo); // reselecting the same player retains the undo
    SelectControlsPlayer(page,1);
    assert(!page.undo && g_selected_player==1);
    SelectControlsPlayer(page,0);
    ImGui::CreateContext();auto&io=ImGui::GetIO();io.DisplaySize={1200,900};io.DeltaTime=1.0F/60;
    io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
    assert(FriendlyBinding("a")=="Cross" && FriendlyBinding("x")=="Square");
    assert(FriendlyBinding("back")=="Touchpad" && FriendlyBinding("start")=="Options");
    assert(FriendlyBinding("lefttrigger +")=="L2" && FriendlyBinding("righttrigger +")=="R2");
    frame(true);frame();
    assert(GImGui->NavId==fields[0].id);
    assert(saved==0 && controller_preset(0)==ControllerPreset::Classic);
    // Keyboard-focus initialization starts without gamepad highlighting.
    // Enter real gamepad navigation before exercising activation.
    press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);
    assert(GImGui->NavId==fields[0].id);
    press(ImGuiKey_GamepadFaceDown);
    assert(saved==1 && page.undo && controller_preset(0)==ControllerPreset::TriggerDriving);
    RestoreControlsSnapshot(*page.undo);page.undo.reset();frame();
    assert(controller_preset(0)==ControllerPreset::Classic);
    assert(fields.size()==43); // two presets, 20 pairs, restore bindings
    for(std::size_t n=1;n<fields.size();++n) {
        assert(fields[n].min.y >= fields[n-1].max.y);
    }
    press(ImGuiKey_GamepadDpadDown);
    assert(GImGui->NavId==fields[1].id && saved==1);
    for(std::size_t n=2;n<fields.size();++n) {
        press(ImGuiKey_GamepadDpadDown);
        assert(GImGui->NavId==fields[n].id);
        if(n==2) {
            page.undo=TakeControlsSnapshot();
            press(ImGuiKey_GamepadFaceDown);
            assert(g_capture_action==static_cast<int>(Action::StickUp) && !g_capture_secondary_controller);
            assert(!page.undo && g_capture_popup_pending && !g_capture_finished);
            assert(g_capture_device==CaptureDevice::Controller);
        }
        if(n==3) {
            press(ImGuiKey_GamepadFaceDown);
            assert(g_capture_action==static_cast<int>(Action::StickUp) && g_capture_secondary_controller);
        }
    }
    assert(saved==1); // navigation and capture alone do not change mappings
    assert(GImGui->NavWindow->Scroll.y>0);
    press(ImGuiKey_GamepadDpadUp);
    assert(GImGui->NavId==fields[41].id);
    ImGui::DestroyContext();
    std::puts("PASS actual controls preset/undo and binding rows: PlayStation labels, one-column geometry, D-pad order, scroll, Cross capture and reverse navigation");
}
