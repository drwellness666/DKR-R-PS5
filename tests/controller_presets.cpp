// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the real runtime input, settings records and controls undo snapshot.
#include "runtime_input.hpp"
#include "ps5_profile_paths.hpp"
#include <SDL.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <set>
#include <fstream>
#include <iterator>

namespace dkr::runtime::enhancements {
static bool modern = false;
bool modern_presentation_enabled() { return modern; }
}
namespace dkr::runtime::platform {
auto controller_assignment_keys() { return std::array<std::string, 4>{}; }
}
#include "controls_snapshot.inc"
#include "binding_settings.inc"
#include "shortcut_settings.inc"

static std::string save_bindings() {
    std::ostringstream output;
    #include "save_bindings.inc"
    SaveShortcutSettings(output);
    return output.str();
}
static void reload_bindings(const std::string& records) {
    std::istringstream input(records);
    std::string line;
    ControlsShortcutBindings keyboard{},controller{},alternate{};
    for(std::size_t i=0;i<keyboard.size();++i){auto action=static_cast<dkr::runtime::input::ShortcutAction>(i);keyboard[i]=dkr::runtime::input::shortcut_keyboard_binding(action);controller[i]=dkr::runtime::input::shortcut_controller_binding(action);alternate[i]=dkr::runtime::input::alternate_shortcut_controller_binding(action);}
    while (std::getline(input, line)) {
        const auto equal = line.find('=');
        assert(equal != std::string::npos);
        if(line.starts_with("shortcut.")){assert(LoadShortcutSetting(line.substr(0,equal),std::stoi(line.substr(equal+1)),keyboard,controller,alternate));continue;}
        if (line.find(".controller_key=") == std::string::npos) {
            assert(LoadPlayerBindingSetting(line.substr(0, equal), std::stoi(line.substr(equal + 1))));
        }
    }
    for(std::size_t i=0;i<keyboard.size();++i){auto action=static_cast<dkr::runtime::input::ShortcutAction>(i);dkr::runtime::input::set_shortcut_keyboard_binding(action,keyboard[i]);dkr::runtime::input::set_shortcut_controller_binding(action,controller[i]);dkr::runtime::input::set_alternate_shortcut_controller_binding(action,alternate[i]);}
}
static void unique_sources(std::size_t player) {
    using namespace dkr::runtime::input;
    std::set<int> used;
    // Camera orbit deliberately shares the C-button stick in a different mode.
    for (std::size_t index = 0; index <= static_cast<std::size_t>(Action::Accelerate); ++index) {
        auto action = static_cast<Action>(index);
        for (int source : {controller_binding(player, action), secondary_controller_binding(player, action)}) {
            if (source != kUnbound) assert(used.insert(source).second);
        }
    }
}
static void legacy_preview_layout(std::size_t player) {
    using namespace dkr::runtime::input;
    assert(apply_controller_preset(player, ControllerPreset::Classic));
    set_secondary_controller_binding(player, Action::A, 1000 + SDL_CONTROLLER_AXIS_TRIGGERRIGHT * 2 + 1);
    set_secondary_controller_binding(player, Action::B, 1000 + SDL_CONTROLLER_AXIS_TRIGGERLEFT * 2 + 1);
    set_controller_binding(player, Action::Z, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    set_controller_binding(player, Action::L, SDL_CONTROLLER_BUTTON_Y);
    set_controller_binding(player, Action::Accelerate, kUnbound);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    using namespace dkr::runtime::input;
    reset_defaults();
    assert(controller_preset(0) == ControllerPreset::Classic);
    set_keyboard_binding(0, Action::A, SDL_SCANCODE_F);
    set_controller_binding(1, Action::A, SDL_CONTROLLER_BUTTON_Y);
    const auto original = TakeControlsSnapshot();
    auto overlay = shortcut_controller_binding(ShortcutAction::ToggleOverlay);
    legacy_preview_layout(0);
    assert(controller_preset(0) == ControllerPreset::Custom);
    assert(migrate_removed_ps5_driving_layout(0));
    assert(!migrate_removed_ps5_driving_layout(0));
    assert(!migrate_removed_ps5_driving_layout(kPlayerCount));
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(keyboard_binding(0, Action::A) == SDL_SCANCODE_F);
    assert(controller_binding(1, Action::A) == SDL_CONTROLLER_BUTTON_Y);
    assert(shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary == overlay.primary);
    for (std::size_t player = 0; player < kPlayerCount; ++player) unique_sources(player);
    assert(!apply_controller_preset(kPlayerCount, ControllerPreset::Classic));
    assert(!apply_controller_preset(0, ControllerPreset::Custom));
    assert(!apply_controller_preset(0, static_cast<ControllerPreset>(2))); // removed preset
    assert(!apply_controller_preset(0, static_cast<ControllerPreset>(255)));
    assert(controller_preset(0) == ControllerPreset::Classic);
    // A single customized primary or alternate must prevent migration of the
    // entire map; do not erase an intentional user binding.
    for (std::size_t index = 0; index < action_count(); ++index) {
        const auto action = static_cast<Action>(index);
        for (bool secondary : {false, true}) {
            legacy_preview_layout(0);
            if (secondary) set_secondary_controller_binding(0, action, 7000);
            else set_controller_binding(0, action, 7000);
            const auto before = save_bindings();
            assert(!migrate_removed_ps5_driving_layout(0));
            assert(save_bindings() == before);
        }
    }
    assert(apply_controller_preset(0, ControllerPreset::Classic));

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
        assert(sample(0, 0, -1).buttons == 0);
        // Gas uses the face button again. Verify repeated press/release edges
        // through both actual SDL polling and the snapshot path.
        for (int tap = 0; tap < 8; ++tap) {
            assert(sample(0, 0, SDL_CONTROLLER_BUTTON_A).buttons == 0x8000);
            assert(sample(0, 0, -1).buttons == 0);
        }
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_X).buttons == 0x4000);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_LEFTSHOULDER).buttons == 0x0020);
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER).buttons == 0x0010);
        assert(sample(32767, 0, -1).buttons == 0x2000); // original Z / item on L2
        assert(sample(0, 32767, -1).buttons == 0); // no extra N64 gas binding
        assert(sample(0, 0, SDL_CONTROLLER_BUTTON_START).buttons == 0x1000);
        assert(sample(32767, 32767, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, true).buttons == 0);
        assert(poll(0, nullptr, nullptr, false, false).buttons == 0);
        assert(poll_snapshot(0, nullptr, nullptr, false, false).buttons == 0);
        sample(0, 0, SDL_CONTROLLER_BUTTON_BACK);
        assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0, 0, -1);
        assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    }
    // Independent alternates use the same edge detector and source suppression
    // in real SDL polling and the native snapshot path.
    set_alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay,{SDL_CONTROLLER_BUTTON_Y,kUnbound});
    for(bool modern:{false,true}){
        dkr::runtime::enhancements::modern=modern;
        sample(0,0,-1);assert(sample(0,0,SDL_CONTROLLER_BUTTON_Y).buttons==0);
        assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0,0,SDL_CONTROLLER_BUTTON_Y);assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0,0,-1);sample(0,0,SDL_CONTROLLER_BUTTON_BACK);assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0,0,-1);sample(0,0,SDL_CONTROLLER_BUTTON_Y,true);assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
        sample(0,0,-1);
    }
    dkr::runtime::controllers::ControllerSnapshot chord;chord.connected=true;chord.button_bound.fill(1);
    set_alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay,{SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_X});
    chord.buttons[SDL_CONTROLLER_BUTTON_A]=1;
    assert(poll_snapshot(0,&chord,nullptr,false,false).buttons!=0);
    assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    chord.buttons[SDL_CONTROLLER_BUTTON_X]=1;
    assert(poll_snapshot(0,&chord,nullptr,false,false).buttons==0);
    assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
    chord.buttons.fill(0);poll_snapshot(0,&chord,nullptr,false,false);
    chord.buttons[SDL_CONTROLLER_BUTTON_A]=1;chord.buttons[SDL_CONTROLLER_BUTTON_X]=1;
    assert(poll_snapshot(1,&chord,nullptr,false,false,true,false).buttons!=0);
    assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    set_alternate_shortcut_controller_binding(ShortcutAction::RecenterGyro,{SDL_CONTROLLER_BUTTON_X,kUnbound});
    const auto shortcuts_saved=save_bindings();reset_controller_shortcuts();reload_bindings(shortcuts_saved);
    assert(alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).primary==SDL_CONTROLLER_BUTTON_A);
    assert(alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay).secondary==SDL_CONTROLLER_BUTTON_X);
    assert(alternate_shortcut_controller_binding(ShortcutAction::RecenterGyro).primary==SDL_CONTROLLER_BUTTON_X);
    assert(save_bindings()==shortcuts_saved);
    // Capture/restore cannot fire a newly assigned control while it is still held.
    set_alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay,{});
    sample(0,0,-1);sample(0,0,SDL_CONTROLLER_BUTTON_X);
    set_alternate_shortcut_controller_binding(ShortcutAction::ToggleOverlay,{SDL_CONTROLLER_BUTTON_X,kUnbound});
    sample(0,0,SDL_CONTROLLER_BUTTON_X);assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    sample(0,0,SDL_CONTROLLER_BUTTON_X);assert(!consume_shortcut_request(ShortcutAction::ToggleOverlay));
    sample(0,0,-1);sample(0,0,SDL_CONTROLLER_BUTTON_X);assert(consume_shortcut_request(ShortcutAction::ToggleOverlay));
    sample(0,0,-1);
    // Real saved record format and loader, no new preset setting/file.
    auto saved = save_bindings();
    reset_defaults();
    reload_bindings(saved);
    assert(save_bindings() == saved);
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(controller_preset(1) == ControllerPreset::Custom);
    // Exercise the actual authored settings writer/loader through the common
    // path, in both directions, without inventing a second binding format.
    const std::filesystem::path root = argv[1];
    const auto profile10 = root / "profile-us-v77";
    const auto profile11 = root / "profile-us-v80";
    std::filesystem::create_directories(profile10);
    std::filesystem::create_directories(profile11);
    const auto config10 = dkrr::ps5_profiles::settings(profile10, root);
    const auto config11 = dkrr::ps5_profiles::settings(profile11, root);
    const auto path10 = config10 / "dkr-port-settings.ini";
    const auto path11 = config11 / "dkr-port-settings.ini";
    { std::ofstream file(path10); file << save_bindings(); assert(file.good()); }
    reset_defaults();
    { std::ifstream file(path11); reload_bindings({std::istreambuf_iterator<char>(file), {}}); }
    assert(save_bindings() == saved);
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(controller_preset(1) == ControllerPreset::Custom);
    assert(apply_controller_preset(0, ControllerPreset::Classic));
    const auto from11 = save_bindings();
    { std::ofstream file(path11); file << from11; assert(file.good()); }
    reset_defaults();
    { std::ifstream file(path10); reload_bindings({std::istreambuf_iterator<char>(file), {}}); }
    assert(save_bindings() == from11);
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(controller_preset(1) == ControllerPreset::Custom);
    RestoreControlsSnapshot(original);
    assert(controller_preset(0) == ControllerPreset::Classic);
    assert(controller_binding(1, Action::A) == SDL_CONTROLLER_BUTTON_Y);
    assert(keyboard_binding(0, Action::A) == SDL_SCANCODE_F);
    for (std::size_t player = 0; player < kPlayerCount; ++player) {
        legacy_preview_layout(player);
        assert(migrate_removed_ps5_driving_layout(player));
        unique_sources(player);
        assert(controller_preset(player) == ControllerPreset::Classic);
    }
    // Saved legacy maps migrate after loading and remain Classic when written
    // and read again through the same shared settings path.
    legacy_preview_layout(0);
    saved = save_bindings();
    reset_defaults(); reload_bindings(saved);
    assert(migrate_removed_ps5_driving_layout(0));
    saved = save_bindings();
    reset_defaults(); reload_bindings(saved);
    assert(!migrate_removed_ps5_driving_layout(0));
    assert(save_bindings() == saved);
    SDL_GameControllerClose(controller);
    assert(SDL_JoystickDetachVirtual(index) == 0);
    SDL_Quit();
    std::puts("PASS real SDL and snapshot inputs: classic face-button gas taps, L2 items, removed-layout migration, custom-map preservation, neutral pause/disconnect, shared settings round-trip and actual undo");
}
