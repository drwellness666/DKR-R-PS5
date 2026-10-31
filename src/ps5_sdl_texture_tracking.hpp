// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL.h>
#include <cstdint>
namespace dkrr::sdl_textures {
// A monotonically increasing version distinguishes recycled SDL addresses and
// live pixel updates. GPU cache entries are never keyed by an address alone.
std::uint64_t version(SDL_Texture* texture);
}
