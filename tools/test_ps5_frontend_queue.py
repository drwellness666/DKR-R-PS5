#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reproduce the deferred software clears with the exact pinned SDL library."""
from pathlib import Path
import argparse,os,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('host_sdl_build',type=Path);a=p.parse_args();s=r/'work/vrr-reset-refinement/sdl/src/ps5-SDL';b=a.host_sdl_build.resolve()
with tempfile.TemporaryDirectory(prefix='dkrr-menu-queue-') as folder:
 t=Path(folder);source=r'''#include "SDL_internal.h"
#include "render/SDL_sysrender.h"
#include "ps5_sdl_texture_pixels.hpp"
#include <cassert>
#include <cstdio>
static unsigned clears=0;
static int (*original)(SDL_Renderer*,SDL_RenderCommand*,void*,size_t)=nullptr;
static int run(SDL_Renderer*r,SDL_RenderCommand*c,void*v,size_t n){for(auto*p=c;p;p=p->next)if(p->command==SDL_RENDERCMD_CLEAR)++clears;return original(r,c,v,n);}
static unsigned pending(SDL_Renderer*r){unsigned n=0;for(auto*p=r->render_commands;p;p=p->next)++n;return n;}
int main(){SDL_SetHint(SDL_HINT_RENDER_BATCHING,"1");assert(SDL_Init(SDL_INIT_VIDEO)==0);auto*w=SDL_CreateWindow("fixture",0,0,64,64,SDL_WINDOW_HIDDEN);assert(w);auto*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);assert(r&&r->batching);original=r->RunCommandQueue;r->RunCommandQueue=run;SDL_RenderFlush(r);
 auto*image=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,8,8);assert(image);unsigned char rgba[8*8*4];for(auto&v:rgba)v=255;assert(SDL_UpdateTexture(image,nullptr,rgba,8*4)==0);SDL_RenderFlush(r);
 for(unsigned frame=0;frame<5000;++frame){SDL_SetRenderDrawColor(r,6,9,14,255);SDL_RenderClear(r);}assert(pending(r)>=5000&&clears==0);
 std::vector<unsigned char>out;int width=0,height=0;assert(dkrr::sdl_textures::pixels(r,image,out,width,height));assert(width==8&&height==8&&clears==5000);assert(out==std::vector<unsigned char>(rgba,rgba+sizeof(rgba)));
 SDL_RenderFlush(r);clears=0;
 for(unsigned frame=0;frame<5000;++frame){/* Native Vulkan owns the frame clear; no software draw queued. */}
 assert(pending(r)==0);assert(dkrr::sdl_textures::pixels(r,image,out,width,height));assert(clears==0&&out==std::vector<unsigned char>(rgba,rgba+sizeof(rgba)));
 SDL_DestroyTexture(image);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();puts("PASS exact SDL2.30.12: 5000 unused software clears accumulate and execute on the actual native image-readback helper; GPU-only frame path leaves no clear backlog and identical image pixels");}
''';(t/'test.cpp').write_text(source)
 subprocess.run(['clang++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(b/'include'),'-I'+str(s/'include'),'-I'+str(s/'src'),'-I'+str(r/'src'),str(t/'test.cpp'),str(b/'libSDL2.a'),'-lm','-ldl','-pthread','-lrt','-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test')],check=True,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'))
# Check the production call site, not just the reproducer's clean path.
ui=(r/'work/upstream-109-20261024/private/platform-rebase/runtime-recomp/src/game/runtime_ui.cpp').read_text();at=ui.index('        SDL_SetRenderDrawColor(renderer, 6, 9, 14, 255);');before=ui.rfind('#if !defined(__PROSPERO__)',0,at);after=ui.index('#endif',at)
assert before>=0 and '#endif' not in ui[before:at] and 'SDL_RenderClear(renderer);' in ui[at:after]
print('PASS production PS5 preprocessor guard excludes the unused software framebuffer clear')
