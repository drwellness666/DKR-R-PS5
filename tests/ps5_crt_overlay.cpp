// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui.h>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
struct Image {unsigned width,height;ImTextureID image_id;};
enum class ScaleMode {Stretch,Tile};
static bool draw(const Image& image,ScaleMode scale_mode,float strength){
#include "crt_draw.inc"
static void frame(float width,float height,unsigned tw,unsigned th,ScaleMode mode,bool optimized) {
 auto& io=ImGui::GetIO();io.DisplaySize={width,height};ImGui::NewFrame();
 Image image{tw,th,reinterpret_cast<ImTextureID>(1)};assert(draw(image,mode,0.75F));ImGui::Render();
 const auto* data=ImGui::GetDrawData();assert(data);
 const unsigned expected=mode==ScaleMode::Stretch||optimized?1U:unsigned(std::ceil(width/tw))*unsigned(std::ceil(height/th));
 assert(data->TotalVtxCount==int(expected*4)&&data->TotalIdxCount==int(expected*6));
 // Every actual quad must sample the same periodic image as its screen-space
 // position, including a partial tile at the bottom/right edge.
 for(int list=0;list<data->CmdListsCount;++list){const auto* dl=data->CmdLists[list];for(int v=0;v<dl->VtxBuffer.Size;v+=4){
  const auto a=dl->VtxBuffer[v],b=dl->VtxBuffer[v+2];
  for(float f:{0.125F,0.5F,0.875F}){
   const float px=a.pos.x+(b.pos.x-a.pos.x)*f,py=a.pos.y+(b.pos.y-a.pos.y)*f;
   float u=a.uv.x+(b.uv.x-a.uv.x)*f,vv=a.uv.y+(b.uv.y-a.uv.y)*f;
   const float eu=mode==ScaleMode::Stretch?px/width:px/tw,ev=mode==ScaleMode::Stretch?py/height:py/th;
   const auto periodic=[](float x){return x-std::floor(x);};
   const float du=std::abs(periodic(u)-periodic(eu)),dv=std::abs(periodic(vv)-periodic(ev));
   assert(std::min(du,1-du)<0.001F&&std::min(dv,1-dv)<0.001F);
  }
 }}
 std::printf("geometry %gx%g mask=%ux%u mode=%d vertices=%d indices=%d\n",width,height,tw,th,int(mode),data->TotalVtxCount,data->TotalIdxCount);
}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.0F/60;
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
#if defined(__PROSPERO__)
 constexpr bool optimized=true;
#else
 constexpr bool optimized=false;
#endif
 frame(64,64,1,1,ScaleMode::Tile,optimized);
 frame(3840,2160,1920,1080,ScaleMode::Tile,optimized);
 frame(127.5F,93.75F,17,11,ScaleMode::Tile,optimized);
 frame(3840,2160,640,480,ScaleMode::Stretch,optimized);
#if defined(__PROSPERO__)
 frame(3840,2160,1,1,ScaleMode::Tile,true);
 frame(7680,4320,1,1,ScaleMode::Tile,true);
#endif
 ImGui::DestroyContext();puts("PASS real ImGui CRT geometry and periodic UV equivalence; native repeat sampling geometry is bounded");
}
