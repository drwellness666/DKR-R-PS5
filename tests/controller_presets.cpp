// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the real runtime input, settings records and controls undo snapshot.
#include "runtime_input.hpp"
#include <SDL.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <set>

namespace dkr::runtime::enhancements {
static bool modern = false;
bool modern_presentation_enabled() { return modern; }
}
namespace dkr::runtime::platform {
auto controller_assignment_keys() { return std::array<std::string, 4>{}; }
}
#include "controls_snapshot.inc"
#include "binding_settings.inc"

static std::string save_bindings() {
    std::ostringstream output;
    #include "save_bindings.inc"
    return output.str();
}
static void reload_bindings(const std::string& records) {
    std::istringstream input(records);
    std::string line;
    while (std::getline(input, line)) {
        const auto equal = line.find('=');
        assert(equal != std::string::npos);
        if (line.find(".controller_key=") == std::string::npos) {
            assert(LoadPlayerBindingSetting(line.substr(0, equal), std::stoi(line.substr(equal + 1))));
        }
    }
}
static void unique_sources(std::size_t player) {
    using namespace dkr::runtime::input;
    std::set<int> used;
    for (std::size_t index = 0; index < action_count(); ++index) {
        auto action = static_cast<Action>(index);
        for (int source : {controller_binding(player, action), secondary_controller_binding(player, action)}) {
            if (source != kUnbound) assert(used.insert(source).second);
        }
    }
}
int main() {
    using namespace dkr::runtime::input;
    reset_defaults();
    assert(controller_preset(0) == ControllerPreset::Classic);
    set_keyboard_binding(0, Action::A, SDL_SCANCODE_F);
    set_controller_binding(1, Action::A, SDL_CONTROLLER_BUTTON_Y);
    const auto original = TakeControlsSnapshot();
    auto overlay = shortcut_controller_binding(ShortcutAction::ToggleOverlay);
    assert(apply_controller_preset(0, ControllerPreset::TriggerDriving));
    assert(controller_preset(0) == ControllerPreset::TriggerDriving);
    assert(keyboard_binding(0, Action::A) == SDL_SCANCODE_F);
    assert(controller_binding(1, Action::A) == SDL_CONTROLLER_BUTTON_Y);
    assert(shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary == overlay.primary);
    for (std::size_t player = 0; player < kPlayerCount; ++player) unique_sources(player);
    assert(!apply_controller_preset(kPlayerCount, ControllerPreset::Classic));
    assert(!apply_controller_preset(0, ControllerPreset::Custom));
    assert(!apply_controller_preset(0, static_cast<ControllerPreset>(255)));
    assert(controller_preset(0) == ControllerPreset::TriggerDriving);

    assert(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) == 0);
    SDL_JoystickID index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    assert(index >= 0 && SDL_IsGameController(index));
    auto* controller = SDL_GameControllerOpen(index);
    assert(controller);
    auto* joystick = SDL_GameControllerGetJoystick(controller);
    const auto sample = [&](int l2, int r2, int button, bool blocked = false) {
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
            int value = axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? l2
                      : axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT ? r2 : 0;
            assert(SDL_JoystickSetVirtualAxis(joystick, axis,
                static_cast<Sint16>(axis >= SDL_CONTROLLER_AXIS_TRIGGERLEFT
                    ? (value == 0 ? -32768 : value * 2 - 32767) : value)) == 0);
        }
        for (int n = 0; n < SDL_CONTROLLER_BUTTON_MAX; ++n) {
            assert(SDL_JoystickSetVirtualButton(joystick, n, n == button) == 0);
        }
        SDL_JoystickUpdate();
        assert(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == l2);
        assert(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == r2);
        dkr::runtime::controllers::ControllerSnapshot snapshot;
        snapshot.connected = true;
        snapshot.button_bound.fill(1);
        snapshot.axes[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = static_cast<Sint16>(l2);
        snapshot.axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = static_cast<Sint16>(r2);
        if (button >= 0) snapshot.buttons[button] = 1;
        const auto direct = poll(0, controller, nullptr, false, blocked);
        const auto isolated = poll_snapshot(0, &snapshot, nullptr, false, blocked);
        assert(direct.buttons == isolated.buttons);
        assert(direct.accelerate == isolated.accelerate);
        assert(direct.stick_x == isolated.stick_x && direct.stick_y == isolated.stick_y);
        return direct;
    };
    for (bool modern : {false, true}) {
        dkr::runtime::enhancements::modern = modern;
        // The preset uses original digital A/B in both Accurate and Modern.
        set_trigger_threshold(0.05F);
        assert(sample(0, 0, -1).buttons == 0);
        // Preserve upstream axis deadzone/normalization as well as the N64
        // digital half-value threshold, which is not half physical travel.
        const int deadzone = modern ? static_cast<int>(std::lround(stick_deadzone() * 32767.0F / 100.0F)) : 7849;
        const int edge = (deadzone + 32767) / 2;
        assert(sample(0, edge, -1).buttons == 0);
        assert(sample(edge, 0, -1).buttons == 0);
        assert(sample(0, edge + 1, -1).buttons == 0x8000);
        assert(sample(edge + 1, 0, -1).buttons == 0x4000);
        assert(sample(32767, 32767, -1).buttons == 0xC000);
        assert(!sample(0, 32767, -1).accelerate);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_A).buttons == 0x8000);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_X).buttons == 0x4000);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_LEFTSHOULDER).buttons == 0x2000);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER).buttons == 0x0010);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_Y).buttons == 0x0020);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_START).buttons == 0x1000);
        assert(sample(32767, 32767, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, true).buttons == 0);
        assert(poll(0, nullptr, nullptr, false, false).buttons == 0);
        assert(poll_snapshot(0, nullptr, nullptr, false, false).buttons == 0);
        sample(0, 0, SDL_CONTROLLER_BUTTON_BACK);
        assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0, 0, -1);
        assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    }
    // Real saved record format and loader, no new preset setting/file.
    auto saved = save_bindings();
    reset_defaults();
    reload_bindings(saved);
    assert(save_bindings() == saved);
    assert(controller_preset(0) == ControllerPreset::TriggerDriving);
    assert(controller_preset(1) == ControllerPreset::Custom);
    RestoreControlsSnapshot(original);
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(controller_binding(1, Action::A) == SDL_CONTROLLER_BUTTON_Y);
    assert(keyboard_binding(0, Action::A) == SDL_SCANCODE_F);
    for (std::size_t player = 0; player < kPlayerCount; ++player) {
        assert(apply_controller_preset(player, ControllerPreset::TriggerDriving));
        unique_sources(player);
        assert(apply_controller_preset(player, ControllerPreset::Classic));
        unique_sources(player);
    }
    SDL_GameControllerClose(controller);
    assert(SDL_JoystickDetachVirtual(index) == 0);
    SDL_Quit();
    std::puts("PASS real SDL and snapshot inputs: trigger thresholds, A/B/Z/L/R/Start, overlay, neutral pause/disconnect, unique per-player mappings, settings round-trip and actual undo");
}
