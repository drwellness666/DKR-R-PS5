// SPDX-License-Identifier: GPL-3.0-or-later
// Reuse the frontend's Game ROM picker and starting lights in the pause page.
static std::vector<RomCatalogEntry> catalog;
static std::filesystem::path selected;
static std::string status;
static bool ready=false, unused_launch=false;
static int last_frame=-100;
static ImGuiContext* context=nullptr;
static double arrived=-10.0;
const int frame=ImGui::GetFrameCount();
const bool entered=context!=ImGui::GetCurrentContext() || last_frame!=frame-1;
last_frame=frame;
if(entered) {
    context=ImGui::GetCurrentContext();
    catalog=LoadRomCatalog();
    const unsigned preferred=dkrr_ps5_initial_revision();
    const std::filesystem::path wanted=preferred==80?"/data/dkrr/roms/dkr.us.v80.z64":"/data/dkrr/roms/dkr.us.v77.z64";
    const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const auto& item){return item.path==wanted;});
    if(found!=catalog.end()) ready=SelectCatalogRom(*found,selected,catalog,status);
    else ready=false;
}
const unsigned active=dkrr_ps5_active_revision();
const bool locked=dkr::runtime::netplay::session().active();
const PlayPageContext selection{selected,status,ready,catalog,unused_launch};
const auto* previous_selection=g_launcher_rom;
g_launcher_rom=&selection;
SettingsCard(content_width,[&](float inner) {
    const auto at=ImGui::GetCursorScreenPos();
    DrawPlayLights(ImGui::GetWindowDrawList(),at,ready,arrived);
    const float inset=70.0F;
    ImGui::SetCursorScreenPos({at.x+inset,at.y});
    ImGui::BeginGroup();
    ImGui::TextUnformatted(ready?"READY TO RACE":"ROM UNAVAILABLE");
    ImGui::Text("Playing: US %s",active==80?"1.1 - Rev A":"1.0 - Original");
    const auto picked=DrawRomSelect("pause-game-rom",std::max(inner-inset,1.0F),false,locked,
                                   "Leave the online lobby before changing ROM.");
    if(picked==RomSelectResult::Switched)arrived=PaddockClock();
    ImGui::EndGroup();
    const float group_height=ImGui::GetItemRectSize().y;
    ImGui::SetCursorScreenPos({at.x,at.y+std::max(112.0F,group_height)+14.0F});
    const unsigned target=selected.filename()=="dkr.us.v80.z64"?80U:77U;
    if(ready&&target!=active) {
        if(SettingsRaceButton("LOAD SELECTED VERSION",locked)) {
            // The main thread joins the old runtime, retires its save owner,
            // then starts the selected revision in the same native title.
            g_native_offline_launch_requested.store(true,std::memory_order_release);
            g_native_frontend_page.store(kPagePlay);
            g_native_online_lobby_requested.store(true,std::memory_order_release);
            g_lifecycle_request.store(dkr::runtime::ui::LifecycleRequest::StopGame,std::memory_order_release);
            g_overlay_visible.store(false,std::memory_order_release);
        }
        SettingsHelp("Starts the selected version inside DKR-R. Settings are shared; saved progress stays separate for each version.",inner);
    } else SettingsHelp("Both installed ROMs remain available in Play, Online and Mods / Hacks.",inner);
});
g_launcher_rom=previous_selection;
