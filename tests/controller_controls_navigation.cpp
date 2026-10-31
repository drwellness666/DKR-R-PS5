// SPDX-License-Identifier: GPL-3.0-or-later
// Real preset and binding-row functions, with Dear ImGui gamepad navigation.
// Font styling/device discovery are replaced by fixture presentation helpers.
#include "runtime_input.hpp"
#include <SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <chrono>
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
static float PaddockMeasure(PaddockType,const char*text){return ImGui::CalcTextSize(text).x;}
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
constexpr int kShortcutCaptureAction=-2;
static bool g_capture_shortcut_alternate=false;
static auto g_capture_shortcut_action=dkr::runtime::input::ShortcutAction::Count;
static std::array<int,2> g_shortcut_capture_sources{};
static int g_shortcut_capture_count=0;
static std::chrono::steady_clock::time_point g_shortcut_capture_deadline{};
static int saved=0;
static void SaveSettings(){++saved;}
#include "shortcut_names.inc"
#include "shortcut_begin.inc"
#include "shortcut_commit.inc"
#include "controls_row.inc"
#include "controls_bindings.inc"
static ControlsPageState& page=g_controls_page;
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
    for(const auto& [source,name]:std::array<std::pair<const char*,const char*>,12>{{
        {"leftx -","Left stick Left"},{"leftx +","Left stick Right"},
        {"lefty -","Left stick Up"},{"lefty +","Left stick Down"},
        {"rightx -","Right stick Left"},{"rightx +","Right stick Right"},
        {"righty -","Right stick Up"},{"righty +","Right stick Down"},
        {"dpup","D-pad Up"},{"dpdown","D-pad Down"},
        {"dpleft","D-pad Left"},{"dpright","D-pad Right"}}}) assert(FriendlyBinding(source)==name);
    assert(FriendlyBinding("leftx - + a")=="Left stick Left + Cross");
    set_controller_binding(0, Action::A, SDL_CONTROLLER_BUTTON_Y);
    set_controller_binding(1, Action::A, SDL_CONTROLLER_BUTTON_B);
    set_alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay,{SDL_CONTROLLER_BUTTON_X,kUnbound});
    frame(true);frame();
    assert(GImGui->NavId==fields[0].id&&fields.size()==48&&saved==0);
    for(const auto& field:fields){assert(field.name.find("Restore classic")==std::string::npos);assert(field.name.find("R2 / L2 driving")==std::string::npos);}
    for(std::size_t n=0;n<48;n+=2){
        assert(fields[n].min.y==fields[n+1].min.y&&fields[n].max.x<fields[n+1].min.x);
        if(n)assert(fields[n].min.y>=fields[n-1].max.y);
    }
    press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);
    for(std::size_t n=0;n<46;n+=2){
        assert(GImGui->NavId==fields[n].id);
        if(n==0){press(ImGuiKey_GamepadFaceDown);assert(g_capture_action==int(Action::StickUp)&&!g_capture_secondary_controller&&g_capture_popup_pending&&!g_capture_finished);}
        if(n==40){press(ImGuiKey_GamepadFaceDown);assert(g_capture_action==kShortcutCaptureAction&&g_capture_shortcut_action==ShortcutAction::ToggleOverlay&&!g_capture_shortcut_alternate);}
        press(ImGuiKey_GamepadDpadRight);assert(GImGui->NavId==fields[n+1].id);
        if(n==0){press(ImGuiKey_GamepadFaceDown);assert(g_capture_secondary_controller);}
        if(n==40){
            press(ImGuiKey_GamepadFaceDown);assert(g_capture_shortcut_alternate&&g_capture_device==CaptureDevice::Controller);
            const auto primary=shortcut_controller_binding(ShortcutAction::ToggleOverlay);
            g_shortcut_capture_sources={SDL_CONTROLLER_BUTTON_Y,kUnbound};g_shortcut_capture_count=1;CommitShortcutCapture();
            assert(saved==1&&g_capture_finished&&alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==SDL_CONTROLLER_BUTTON_Y);
            assert(shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==primary.primary);
        }
        press(ImGuiKey_GamepadDpadLeft);press(ImGuiKey_GamepadDpadDown);
    }
    assert(GImGui->NavId==fields[46].id&&saved==1);
    press(ImGuiKey_GamepadFaceDown);assert(saved==2&&page.undo&&controller_preset(0)==ControllerPreset::Classic);
    assert(controller_binding(1,Action::A)==SDL_CONTROLLER_BUTTON_B);
    assert(alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==kUnbound);
    RestoreControlsSnapshot(*page.undo);page.undo.reset();frame();
    assert(controller_preset(0)==ControllerPreset::Custom&&alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==SDL_CONTROLLER_BUTTON_Y);
    press(ImGuiKey_GamepadDpadRight);assert(GImGui->NavId==fields[47].id);press(ImGuiKey_GamepadFaceDown);
    assert(saved==3&&page.undo&&controller_preset(0)==ControllerPreset::Classic&&controller_preset(1)==ControllerPreset::Classic);
    RestoreControlsSnapshot(*page.undo);assert(controller_binding(1,Action::A)==SDL_CONTROLLER_BUTTON_B);
    assert(alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==SDL_CONTROLLER_BUTTON_Y);
    assert(GImGui->NavWindow->Scroll.y>0);
    ImGui::DestroyContext();
    std::puts("PASS actual mapping table: 20 game actions plus 3 shortcut pairs, readable directions, no layout/disclosure block, full D-pad grid and alternate capture, adjacent player/all reset and undo including shortcuts");
}
