// SPDX-License-Identifier: GPL-3.0-or-later
// Isolated real console font loader and ImGui atlas; no ROM, GPU or bundled images.
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "generated/selawik_font.h"
std::uint64_t g_font_generation=0;
int g_race_button_flat=0;ImFont*g_race_button_flat_font=nullptr;
ImFont *g_font_controls=nullptr,*g_font_heading=nullptr,*g_font_title=nullptr,*g_font_fps=nullptr;
#include "runtime_ui_paddock.inl"
void LoadBrandLogoIntoAtlas(){ImGui::GetIO().Fonts->TexDesiredWidth=2048;}
void LoadConsoleFonts(){
#include "ps5_ui_fonts.inl"
}
static void check(bool ok,const char*why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
 ImGui::CreateContext();LoadConsoleFonts();auto&io=ImGui::GetIO();
 unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
 check(p&&w>0&&h>0&&w<=4096&&h<=4096,"bounded populated atlas");
 unsigned checked=0;
 for(auto*font:io.Fonts->Fonts)for(ImWchar c:{ImWchar('A'),ImWchar('0'),ImWchar(0x00FC)}){
  const auto*g=font->FindGlyphNoFallback(c);check(g&&g->Visible,"ASCII, digit or German glyph missing");
  unsigned long long coverage=0;
  for(int y=int(g->V0*h);y<int(g->V1*h);++y)for(int x=int(g->U0*w);x<int(g->U1*w);++x)coverage+=p[(y*w+x)*4+3];
  check(coverage>0,"glyph is fully transparent");++checked;
 }
 check(io.Fonts->Fonts.Size==25,"all expected font banks installed");
 std::printf("PASS: actual console OFL loader, %d font banks, %u visible glyph checks, atlas=%dx%d; no restricted font linked\n",io.Fonts->Fonts.Size,checked,w,h);ImGui::DestroyContext();
}
