// SPDX-License-Identifier: GPL-3.0-or-later
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <cassert>
#include "ps5_revision_selection.h"
static unsigned preferred=77,active=77,available=3;
extern "C" unsigned dkrr_ps5_initial_revision(){return preferred;}
extern "C" unsigned dkrr_ps5_active_revision(){return active;}
extern "C" int dkrr_ps5_choose_revision(unsigned rev){preferred=rev;return 0;}
struct RomCatalogEntry{std::filesystem::path path;};
static std::vector<RomCatalogEntry> LoadRomCatalog(){std::vector<RomCatalogEntry> v{{"/data/dkrr/roms/dkr.us.v77.z64"}};if(available&2)v.push_back({"/data/dkrr/roms/dkr.us.v80.z64"});return v;}
static bool SelectCatalogRom(const RomCatalogEntry&entry,std::filesystem::path&selected,const std::vector<RomCatalogEntry>&,std::string&){selected=entry.path;return true;}
struct PlayPageContext{const std::filesystem::path&selected;const std::string&status;bool ready;const std::vector<RomCatalogEntry>&catalog;bool&launch;};
static const PlayPageContext* g_launcher_rom=nullptr;
static bool online=false;
namespace dkr::runtime::netplay{struct Session{bool active(){return online;}};static Session value;static Session&session(){return value;}}
static constexpr int kPagePlay=0;
static std::atomic<bool> g_native_offline_launch_requested=false,g_native_online_lobby_requested=false;
static std::atomic<int> g_native_frontend_page=-1;
namespace dkr::runtime::ui {enum class LifecycleRequest{None,StopGame};}
static std::atomic<dkr::runtime::ui::LifecycleRequest> g_lifecycle_request=dkr::runtime::ui::LifecycleRequest::None;
static std::atomic<bool> g_overlay_visible=true;
static double PaddockClock(){return ImGui::GetTime();}
static void DrawPlayLights(ImDrawList*,ImVec2,bool,double){}
static void SettingsHelp(const char*label,float){ImGui::TextWrapped("%s",label);}
static bool SettingsRaceButton(const char*label,bool disabled){ImGui::BeginDisabled(disabled);const bool result=ImGui::Button(label);ImGui::EndDisabled();return result&&!disabled;}
template<class F>static void SettingsCard(float width,F body){ImGui::BeginGroup();body(width);ImGui::EndGroup();}
enum class RomSelectResult{None,Switched};
static ImGuiID original=0,revision=0,load=0;
// Catalog widget is supplied by upstream; this fixture drives its selection
// outcome and checks the actual PS5 pause-page lifecycle around that widget.
static RomSelectResult DrawRomSelect(const char*,float,bool,bool locked,const char*) {
 assert(g_launcher_rom);auto&context=*g_launcher_rom;RomSelectResult result=RomSelectResult::None;
 ImGui::BeginDisabled(locked);
 for(unsigned i=0;i<context.catalog.size();++i){const auto&entry=context.catalog[i];const char*label=i?"US 1.1":"US 1.0";
  if(ImGui::Button(label)){const_cast<std::filesystem::path&>(context.selected)=entry.path;preferred=i?80:77;result=RomSelectResult::Switched;}
  if(i)revision=ImGui::GetItemID();else original=ImGui::GetItemID();
 }
 ImGui::EndDisabled();return result;
}
#include "sidebar_marker.inc"
static void frame(bool focus=false){
 ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1000,700});ImGui::Begin("Play",nullptr,ImGuiWindowFlags_NoTitleBar);
 const auto vertices=ImGui::GetWindowDrawList()->VtxBuffer.Size;DrawSidebarCurrentStripe();assert(ImGui::GetWindowDrawList()->VtxBuffer.Size==vertices);
 const float content_width=740; if(focus)ImGui::SetKeyboardFocusHere();
 #include "ps5_revision_ui.inl"
 assert(!g_launcher_rom);load=ImGui::GetID("LOAD SELECTED VERSION");ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
int main(){
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1280,800};io.DeltaTime=1.F/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
 unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
 frame(true);frame();assert(GImGui->NavId==original);press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==revision);
 press(ImGuiKey_GamepadFaceDown);assert(preferred==80&&active==77&&!g_native_offline_launch_requested);
 press(ImGuiKey_GamepadDpadDown);assert(GImGui->NavId==load);press(ImGuiKey_GamepadFaceDown);
 assert(g_native_offline_launch_requested&&g_native_frontend_page==kPagePlay&&g_native_online_lobby_requested&&g_lifecycle_request==dkr::runtime::ui::LifecycleRequest::StopGame&&!g_overlay_visible);
 // A live lobby blocks replacement. The previous context is restored after
 // every draw and both installed files remain in the catalog.
 online=true;g_native_offline_launch_requested=false;frame(true);press(ImGuiKey_GamepadFaceDown);assert(!g_native_offline_launch_requested&&LoadRomCatalog().size()==2);
 ImGui::DestroyContext();puts("PASS actual PS5 revision pause page: catalog ownership, controller selection, explicit in-process load request, lobby lock, restored frontend context and no sidebar stripe");
}
