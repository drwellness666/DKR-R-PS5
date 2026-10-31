// SPDX-License-Identifier: GPL-3.0-or-later
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
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
#include "generated/racing_banana_font.h"
#include "generated/jumpman_font.h"
#include "ps5_display_options.h"
#include "ps5_hdr_options.h"
std::uint64_t g_font_generation=0;
int g_race_button_flat=0;ImFont*g_race_button_flat_font=nullptr;
ImFont *g_font_controls=nullptr,*g_font_heading=nullptr,*g_font_title=nullptr,*g_font_fps=nullptr;
#include "runtime_ui_paddock.inl"
void LoadBrandLogoIntoAtlas(){ImGui::GetIO().Fonts->TexDesiredWidth=2048;}
#include "upstream_launcher_fonts.inl"
void OlFade(ImDrawList*,int,float){}
#define SettingsRange ActualSettingsRange
#include "settings.inc"
#undef SettingsRange
static std::vector<ImRect> ranges;
template<class Format>bool SettingsRange(const char* id,std::string_view label,int* value,int lo,int hi,
    float width,Format&& format,std::string_view help={},bool disabled=false){
    const auto start=ImGui::GetCursorScreenPos();
    bool result=ActualSettingsRange(id,label,value,lo,hi,width,std::forward<Format>(format),help,disabled);
    ranges.emplace_back(start,ImVec2{start.x+width,ImGui::GetItemRectMax().y});
    return result;
}
// Unrelated output selectors are layout placeholders; HDR controls, fonts,
// wrapping, gamepad interaction, the full page and disclosures are real.
bool SettingsDropdown(const char* id,std::string_view,int*,std::initializer_list<const char*>,float width,bool,const char*){return ImGui::Button(id,{width,44});}
bool SettingsRaceButton(const char* label,bool){return ImGui::Button(label,{280,44});}
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status(){return {3840,2160,119880,0,1};}
extern "C" int dkrr_ps5_read_output(dkrr_ps5_display_choice*c,dkrr_ps5_hdr_choice*h){*c={2160,120};*h={1,200,800,0};return 1;}
extern "C" int dkrr_ps5_choose_hdr(dkrr_ps5_hdr_choice){return 0;}
extern "C" int dkrr_ps5_choose_output(dkrr_ps5_display_choice,dkrr_ps5_hdr_choice){return 0;}
extern "C" void dkrr_ps5_hdr_preview(dkrr_ps5_hdr_choice){}
extern "C" void dkrr_ps5_hdr_calibration_pattern(unsigned){}
extern "C" struct dkrr_ps5_hdr_status dkrr_ps5_hdr_status(){return {2,0};}
#include "ps5_display_ui.inl"
#include "details.inc"
static float width=640;
static bool details=false;
static unsigned counter=0;
static ImGuiID detail_id=0;
static void frame(bool focus=false){
    ranges.clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({width+40,4000});
    ImGui::Begin("Graphics",nullptr,ImGuiWindowFlags_NoTitleBar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    if(details){
        if(focus)ImGui::SetKeyboardFocusHere();
        DrawTechnicalDetails("Uploaded textures: "+std::to_string(++counter),"Texture diagnostics");
        ImGui::PushID("Texture diagnostics");detail_id=ImGui::GetID("Texture diagnostics");ImGui::PopID();
    }else DrawPs5DisplayFields(width,false);
    ImGui::PopStyleVar();ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
int main(){
    ImGui::CreateContext();LoadLauncherFonts();auto&io=ImGui::GetIO();io.DisplaySize={1200,4500};io.DeltaTime=1.0F/60;
    io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
    g_ps5_display_ui.loaded=true;g_ps5_display_ui.hdr_mode=1;g_ps5_display_ui.hdr_tuning=true;
    for(float tested:{280.0F,640.0F,1000.0F}){
        width=tested;frame();frame();assert(ranges.size()==2);
        for(unsigned i=0;i<2;++i){assert(ranges[i].Max.y>ranges[i].Min.y+44);
            if(i)assert(ranges[i].Min.y>=ranges[i-1].Max.y+18.0F);}
        assert(g_ps5_display_ui.expansion==0);
    }
    details=true;width=640;frame(true);frame();press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);
    auto*window=ImGui::FindWindowByName("Graphics");assert(window);
    window->StateStorage.SetInt(detail_id,1);
    for(int i=0;i<120;++i){frame();assert(window->StateStorage.GetInt(detail_id,0)==1);
        unsigned active_children=0;
        for(auto*child:GImGui->Windows)if(child->ParentWindow==window&&child->Active&&
            (child->Flags&ImGuiWindowFlags_ChildWindow))++active_children;
        assert(active_children==1);}
    ImGui::DestroyContext();puts("PASS real HDR slider geometry at 280/640/1000px with upstream fonts and zero item spacing; changing texture diagnostics retain disclosure state for 120 frames");
}
