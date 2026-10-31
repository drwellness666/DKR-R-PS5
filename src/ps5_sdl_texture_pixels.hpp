// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL.h>
#include <vector>
#include <cstdint>
namespace dkrr::sdl_textures {
// Read a source image only on a cache miss, never the entire menu framebuffer.
// Preserve renderer state and straight alpha, independent of source modulation.
inline bool pixels(SDL_Renderer* renderer,SDL_Texture* texture,std::vector<unsigned char>& out,int& w,int& h) {
    if(SDL_QueryTexture(texture,nullptr,nullptr,&w,&h)||w<=0||h<=0||w>8192||h>8192||std::uint64_t(w)*unsigned(h)>16U*1024U*1024U)return false;
    auto* target=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_BGRA32,SDL_TEXTUREACCESS_TARGET,w,h);if(!target)return false;
    auto* previous=SDL_GetRenderTarget(renderer);SDL_Rect viewport{},clip{};float sx=1,sy=1;
    SDL_RenderGetViewport(renderer,&viewport);SDL_RenderGetClipRect(renderer,&clip);const bool clipped=SDL_RenderIsClipEnabled(renderer)==SDL_TRUE;SDL_RenderGetScale(renderer,&sx,&sy);
    SDL_BlendMode blend=SDL_BLENDMODE_NONE;Uint8 r=255,g=255,b=255,a=255;
    bool good=SDL_GetTextureBlendMode(texture,&blend)==0&&SDL_GetTextureColorMod(texture,&r,&g,&b)==0&&SDL_GetTextureAlphaMod(texture,&a)==0;
    if(good)good=SDL_SetRenderTarget(renderer,target)==0;
    if(good) {
        out.resize(std::size_t(w)*unsigned(h)*4U);
        good=SDL_RenderSetScale(renderer,1,1)==0&&SDL_RenderSetViewport(renderer,nullptr)==0&&SDL_RenderSetClipRect(renderer,nullptr)==0&&
            SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_NONE)==0&&SDL_SetTextureColorMod(texture,255,255,255)==0&&SDL_SetTextureAlphaMod(texture,255)==0&&
            SDL_RenderCopy(renderer,texture,nullptr,nullptr)==0&&SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_BGRA32,out.data(),w*4)==0;
    }
    // Each restoration is attempted even if a prior operation failed.
    bool restored=SDL_SetRenderTarget(renderer,previous)==0;
    restored=(SDL_RenderSetScale(renderer,sx,sy)==0)&&restored;
    restored=(SDL_RenderSetViewport(renderer,&viewport)==0)&&restored;
    restored=(SDL_RenderSetClipRect(renderer,clipped?&clip:nullptr)==0)&&restored;
    restored=(SDL_SetTextureBlendMode(texture,blend)==0)&&restored;
    restored=(SDL_SetTextureColorMod(texture,r,g,b)==0)&&restored;
    restored=(SDL_SetTextureAlphaMod(texture,a)==0)&&restored;
    SDL_DestroyTexture(target);return good&&restored;
}
}
