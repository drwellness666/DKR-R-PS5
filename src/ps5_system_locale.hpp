// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string_view>

extern "C" int sceSystemServiceParamGetInt(int parameter, int* value);

namespace dkrr::ps5_locale {
// Public SystemService language IDs. A language is not a country setting.
// https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/_types/sys_service.h
inline std::string_view system_language() {
    constexpr std::array<std::string_view, 31> languages{
        "ja", "en-US", "fr", "es", "de", "it", "nl", "pt-PT", "ru", "ko",
        "zh-TW", "zh-CN", "fi", "sv", "da", "no", "pl", "pt-BR", "en-GB",
        "tr", "es", "ar", "fr", "cs", "hu", "el", "ro", "th", "vi", "id", "uk"
    };
    int language = -1;
    if (sceSystemServiceParamGetInt(1, &language) != 0 || language < 0 ||
        static_cast<unsigned>(language) >= languages.size()) return "en";
    return languages[static_cast<unsigned>(language)];
}
}
