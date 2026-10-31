// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
// PS5 has no desktop Secret Service. Account tokens remain in the existing
// in-memory upstream session; never invoke payload dlopen from a native title.
namespace dkr::online::keyring {
inline bool available(){return false;}
inline bool store(const std::string&,const std::string&){return false;}
inline std::string load(const std::string&){return {};}
inline void clear(const std::string&){}
}
