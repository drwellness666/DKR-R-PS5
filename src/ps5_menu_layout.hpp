// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
namespace dkrr::ps5_menu {
struct Page { const char* label; int id; };
// Keep 1.0.14's page IDs and grouped Play tools. Native updates own page 11.
inline constexpr std::array<Page,9> pages{{
    {"PLAY",0}, {"DKR-R ONLINE",5}, {"MOD LIBRARY",10}, {"INSTALLED MODS",6},
    {"GRAPHICS",1}, {"SOUND",2}, {"CONTROLS",3}, {"UPDATES",11}, {"ABOUT DKR-R",7}
}};
inline constexpr auto selection_order=[] {
    std::array<int,pages.size()> result{};
    for(unsigned i=0;i<pages.size();++i)result[i]=pages[i].id;
    return result;
}();
}
