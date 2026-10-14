// SPDX-License-Identifier: GPL-3.0-or-later
// Console interface fonts use the upstream unmodified Selawik faces (SIL OFL 1.1).
// This file contains no font bytes and references neither Racing Banana nor Jumpman.
++g_font_generation;
ImGuiIO& io = ImGui::GetIO();
static constexpr ImWchar ranges[] = {0x0020,0x00FF,0x2010,0x2027,0x2190,0x2193,0};
const auto face = [&](float size,bool bold) {
    ImFontConfig config{};
    config.FontDataOwnedByAtlas=false;
    config.OversampleH=2;config.OversampleV=1;
    config.GlyphRanges=ranges;
    const char* data=bold?dkr_selawik_semibold_font:dkr_selawik_regular_font;
    const std::size_t bytes=bold?dkr_selawik_semibold_font_size:dkr_selawik_regular_font_size;
    return io.Fonts->AddFontFromMemoryTTF(const_cast<char*>(data),static_cast<int>(bytes),size,&config,ranges);
};
io.FontDefault=face(21.0F,false);
g_font_controls=io.FontDefault;
g_font_heading=face(58.0F,true);
g_font_title=face(68.0F,true);
g_font_fps=face(24.0F,true);
LoadPaddockReadingFonts(io.Fonts);
LoadPaddockMonoFonts(io.Fonts);
for(std::size_t i=0;i<kPaddockSignSizes.size();++i)
    g_paddock_sign[i]=face(kPaddockSignSizes[i]*g_paddock_reading_scale,true);
LoadBrandLogoIntoAtlas();
unsigned char* pixels=nullptr;int width=0,height=0;
io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
const ImFontGlyph* body_a=io.FontDefault?io.FontDefault->FindGlyph('A'):nullptr;
std::fprintf(stderr,"[ps5][ui] fonts=Selawik-OFL atlas=%dx%d banks=%d body-A=%d\n",width,height,io.Fonts->Fonts.Size,body_a&&body_a->Visible);
