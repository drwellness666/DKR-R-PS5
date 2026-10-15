// SPDX-License-Identifier: GPL-3.0-or-later
// Real upstream capture handler and binding APIs, modeled device ownership.
#include "runtime_input.hpp"
#include <SDL.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
namespace dkr::runtime::enhancements { bool modern_presentation_enabled() { return false; } }
namespace dkr::runtime::platform {
struct MappingProgress { bool capturing=false; };
static MappingProgress controller_mapping_progress() { return {}; }
static void cancel_controller_mapping() {}
static bool handle_controller_mapping_event(SDL_Event*) { return false; }
static int controller_instance_for_player(std::size_t player) { return 100+static_cast<int>(player); }
static bool assign_controller(std::size_t,int) { return true; }
static bool begin_controller_mapping(int,std::size_t) { return false; }
}
enum class CaptureDevice { Keyboard, Controller };
constexpr bool kConsoleMenuLayout=true;
constexpr int kShortcutCaptureAction=-2,kAssignControllerCaptureAction=-3;
static int g_capture_action=0;
static std::size_t g_selected_player=0;
static bool g_capture_finished=false,g_capture_secondary_controller=false;
static CaptureDevice g_capture_device=CaptureDevice::Controller;
static std::array<int,2> g_shortcut_capture_sources{};
static int g_shortcut_capture_count=0;
static std::chrono::steady_clock::time_point g_shortcut_capture_deadline{};
static bool g_controller_mapping_popup_pending=false,g_controller_mapping_completion_saved=false;
static int saves=0;
static void SaveSettings() { ++saves; }
static void CommitShortcutCapture() { g_capture_finished=true; }
#include "controller_capture.inc"
int main() {
    using namespace dkr::runtime::input;
    reset_defaults();
    SDL_Event event{};
    event.type=SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.which=100;
    event.cbutton.button=SDL_CONTROLLER_BUTTON_BACK;
    g_selected_player=1;
    g_capture_action=static_cast<int>(Action::A);
    assert(!HandleInputCaptureEvent(&event)); // another player's touchpad
    assert(!g_capture_finished && saves==0);
    event.cbutton.which=101;
    assert(HandleInputCaptureEvent(&event));
    assert(g_capture_finished && saves==0);
    assert(controller_binding(1,Action::A)==SDL_CONTROLLER_BUTTON_A);
    g_capture_finished=false;
    event.cbutton.button=SDL_CONTROLLER_BUTTON_B;
    assert(HandleInputCaptureEvent(&event));
    assert(g_capture_finished && saves==1);
    assert(controller_binding(1,Action::A)==SDL_CONTROLLER_BUTTON_B); // Circle remains bindable
    g_capture_finished=false;
    g_capture_secondary_controller=true;
    event.type=SDL_CONTROLLERAXISMOTION;
    event.caxis.which=101;
    event.caxis.axis=SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    event.caxis.value=19999;
    assert(!HandleInputCaptureEvent(&event));
    event.caxis.value=25000;
    assert(HandleInputCaptureEvent(&event));
    assert(g_capture_finished && saves==2);
    assert(!g_controller_mapping_popup_pending && !g_controller_mapping_completion_saved);
    assert(secondary_controller_binding(1,Action::A)==encode_controller_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,true));
    g_capture_finished=false;
    g_capture_action=kShortcutCaptureAction;
    event.type=SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.which=101;
    event.cbutton.button=SDL_CONTROLLER_BUTTON_BACK;
    assert(!HandleInputCaptureEvent(&event)); // shortcuts are owned by Player 1
    event.cbutton.which=100;
    assert(HandleInputCaptureEvent(&event));
    assert(g_capture_finished && saves==2);
    std::puts("PASS actual capture handler: owned touchpad cancels without saving, Circle remains bindable, trigger capture threshold and shortcut ownership");
}
