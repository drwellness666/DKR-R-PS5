// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include "launcher_panel_cache.hpp"
#include "launcher_performance.hpp"
#include "launcher_solid_geometry.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <cstdio>
#include <string_view>
#include <vector>
#include "frontend.inc"
int main() {
    for(const auto size:std::array<SDL_Point,3>{{{640,360},{1920,1080},{3840,2160}}}) {
        auto* surface=SDL_CreateRGBSurfaceWithFormat(0,size.x,size.y,32,SDL_PIXELFORMAT_RGBA32);assert(surface);
        auto* renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
        ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={float(size.x),float(size.y)};io.DeltaTime=1.F/60;
        ImGui_ImplSDLRenderer2_Init(renderer);ImGui_ImplSDLRenderer2_NewFrame();
        auto* image=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,1024,1024);assert(image);
        std::vector<Uint32> source(1024*1024,0xff3476bc);assert(!SDL_UpdateTexture(image,nullptr,source.data(),1024*4));
        SDL_SetTextureBlendMode(image,SDL_BLENDMODE_BLEND);
        dkr::runtime::launcher::Profile profile;dkr::runtime::launcher::PanelCache cache;
        std::vector<Uint32> pixels(size.x*size.y),first;
        for(int repeat=0;repeat<3;++repeat) {
            ImGui::NewFrame();auto* draw=ImGui::GetBackgroundDrawList();
            draw->AddRectFilled({0,0},{float(size.x),float(size.y)},IM_COL32(23,74,162,255));
            draw->AddRectFilled({8,8},{float(size.x)/3,float(size.y)-8},IM_COL32(12,32,80,255),12);
            draw->AddImage(reinterpret_cast<ImTextureID>(image),{float(size.x)/2,0},{float(size.x),float(size.y)});
            draw->AddText({20,20},IM_COL32_WHITE,"DKR-R ONLINE");
            ImGui::Render();SDL_SetRenderDrawColor(renderer,1,2,3,255);SDL_RenderClear(renderer);
            RenderLauncherDrawData(renderer,ImGui::GetDrawData(),profile,cache,true);SDL_RenderPresent(renderer);
            assert(!SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGBA32,pixels.data(),size.x*4));
            assert(pixels[(size.y/2)*size.x+size.x/4]==0xff50200c);
            assert(pixels[(size.y/2)*size.x+size.x*3/4]==0xff3476bc);
            assert(pixels[(size.y/2)*size.x+size.x*5/12]==0xffa24a17);
            if(repeat==0)first=pixels;else assert(first==pixels);
        }
        if(size.x<=1920)assert(cache.hits()>0);cache.clear();SDL_DestroyTexture(image);
        ImGui_ImplSDLRenderer2_Shutdown();ImGui::DestroyContext();SDL_DestroyRenderer(renderer);SDL_FreeSurface(surface);
    }
    puts("PASS actual frontend raster: 360p/1080p/4K large solid panels and 1024px image sampling, font rendering, stable repeated frames and active panel cache");
}
