// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui_internal.h>
namespace dkrr::menu_window_policy {
inline ImGuiID owner(){return ImHashStr("##PS5-fixed-menu-panels");}
// ImGui 1.90 binds desktop window selection to held Square. Reserve that
// key only while NewFrame runs internal navigation; release it before the
// game's widgets read input. Do not inject a contradictory button release.
inline void before_navigation(){
    ImGui::SetKeyOwner(ImGuiKey_GamepadFaceLeft,owner(),ImGuiInputFlags_LockUntilRelease);
}
inline void after_navigation(){
    if(ImGui::GetKeyOwner(ImGuiKey_GamepadFaceLeft)==owner())
        ImGui::SetKeyOwner(ImGuiKey_GamepadFaceLeft,ImGuiKeyOwner_None);
}
}
