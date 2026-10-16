// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
namespace dkrr::ps5_menu {
struct Page { const char* label; int id; };
// Stable upstream page IDs; visual and shoulder-button order use one policy.
inline constexpr std::array<Page,9> pages{{
    {"PLAY",0}, {"CONTROLS",3}, {"GRAPHICS",1}, {"SOUND",2},
    {"TEXTURES",8}, {"SAVE MANAGER",4}, {"MODS / HACKS",6},
    {"DKR-R ONLINE",5}, {"ABOUT DKR-R",7}
}};
inline constexpr auto selection_order=[] {
    std::array<int,11> result{};
    for(unsigned i=0;i<pages.size();++i)result[i]=pages[i].id;
    result[9]=9;result[10]=10; // Existing confirmed restart/exit actions.
    return result;
}();
}
