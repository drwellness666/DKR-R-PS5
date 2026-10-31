// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_sdl_texture_tracking.hpp"
#include <mutex>
#include <unordered_map>
namespace {
std::mutex mutex;
std::unordered_map<SDL_Texture*,std::uint64_t> versions;
std::uint64_t next_version=0;
void changed(SDL_Texture* texture) {
    if(!texture)return;
    std::lock_guard lock(mutex);versions[texture]=++next_version;
}
}
namespace dkrr::sdl_textures {
std::uint64_t version(SDL_Texture* texture) {
    std::lock_guard lock(mutex);auto it=versions.find(texture);return it==versions.end()?0:it->second;
}
}
extern "C" {
SDL_Texture* __real_SDL_CreateTexture(SDL_Renderer*,Uint32,int,int,int);
SDL_Texture* __real_SDL_CreateTextureFromSurface(SDL_Renderer*,SDL_Surface*);
int __real_SDL_UpdateTexture(SDL_Texture*,const SDL_Rect*,const void*,int);
void __real_SDL_DestroyTexture(SDL_Texture*);
SDL_Texture* __wrap_SDL_CreateTexture(SDL_Renderer* renderer,Uint32 format,int access,int w,int h) {
    auto* texture=__real_SDL_CreateTexture(renderer,format,access,w,h);changed(texture);return texture;
}
SDL_Texture* __wrap_SDL_CreateTextureFromSurface(SDL_Renderer* renderer,SDL_Surface* surface) {
    auto* texture=__real_SDL_CreateTextureFromSurface(renderer,surface);changed(texture);return texture;
}
int __wrap_SDL_UpdateTexture(SDL_Texture* texture,const SDL_Rect* rect,const void* pixels,int pitch) {
    int rc=__real_SDL_UpdateTexture(texture,rect,pixels,pitch);if(!rc)changed(texture);return rc;
}
void __wrap_SDL_DestroyTexture(SDL_Texture* texture) {
    {std::lock_guard lock(mutex);versions.erase(texture);}
    __real_SDL_DestroyTexture(texture);
}
}
