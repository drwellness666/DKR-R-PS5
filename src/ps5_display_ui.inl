// SPDX-License-Identifier: GPL-3.0-or-later
// Existing Graphics > Game display card; native replacement for Window mode.
struct Ps5DisplayUiState {
    int resolution = 0, refresh = 0;
    bool loaded = false, hdr_tuning=false, hdr_pattern=false;
    int hdr_mode=0, paper_white=200, peak=800, expansion=0;
    std::string status;
};
Ps5DisplayUiState g_ps5_display_ui;
void DrawPs5OutputStatus(float width) {
    const auto active=dkrr_ps5_display_status();
    const auto hdr=dkrr_ps5_hdr_status();
    char line[192];
    if(active.width&&active.height) {
        std::snprintf(line,sizeof(line),"Active output: %u x %u  |  %.2f Hz%s",active.width,active.height,
                      double(active.refresh_millihz)/1000.0,active.fallback?" (fallback)":"");
        SettingsHelp(line,width,0x83E8D0);
    } else SettingsHelp("Active output: waiting for the first game frame.",width);
    SettingsHelp(hdr.output==2?"HDR10: active, HDMI HDR confirmed":
                 hdr.output==1?"HDR10: output buffer active; HDMI confirmation pending":
                 hdr.output==3?"Colour: SDR fallback (HDR could not be activated)":"Colour: SDR",width);
    SettingsHelp("VRR: follows PS5 Automatic and the TV. DKR-R supports VRR without Apply to unsupported games. The TV reports whether VRR is currently active.",width);
    SettingsHelp("ALLM: follows PS5 and TV settings. TV output resolution and game render resolution are separate.",width);
    PaddockGap(12.0F);
}
void DrawPs5DisplayFields(float width, bool entered) {
    auto &ui = g_ps5_display_ui;
    const auto active = dkrr_ps5_display_status();
    if (entered || !ui.loaded) {
        dkrr_ps5_display_choice c{};dkrr_ps5_hdr_choice hdr{};
        const int rc = dkrr_ps5_read_output(&c,&hdr);
        ui.resolution = c.resolution == 2160 ? 2 : c.resolution == 1440 ? 1 : 0;
        ui.refresh = c.refresh == 120 && active.high_refresh_build ? 1 : 0;
        ui.hdr_mode=int(hdr.enabled);ui.paper_white=int(hdr.paper_white);ui.peak=int(hdr.peak);ui.expansion=0;
        ui.loaded = true;
        ui.status = rc < 0 ? "The saved TV output choice could not be read. The safe default was used." : "";
    }
    SettingsDropdown("ps5-tv-size", "TV output resolution", &ui.resolution,
                     {"1080p (default)", "1440p", "4K (2160p)"}, width, false,
                     "Output buffer size. The game render resolution below controls detail separately. Applies on the next start.");
    PaddockGap(10.0F);
    if (active.high_refresh_build) {
        SettingsDropdown("ps5-tv-rate", "TV refresh & smoothness", &ui.refresh,
                         {"60 Hz (default)", "120 Hz"}, width, false,
                         "Modern rendering automatically follows the active 60 or 120 Hz output. Accurate keeps the original frame rate. Unsupported 120 Hz falls back to 60 Hz. Applies after restart; demanding scenes can render more slowly.");
    } else {
        SettingsHelp("TV refresh rate: 60 Hz. A separate optional test build enables the 120 Hz choice.", width);
    }
    PaddockGap(10.0F);
    SettingsDropdown("ps5-hdr-mode","HDR output",&ui.hdr_mode,
                     {"Off (SDR, default)","HDR10"},width,false,
                     "Maps the original SDR game to 10-bit BT.2020 / PQ. Requires PS5 HDR and a compatible TV. Applies on the next start; unsupported output falls back to SDR.");
    PaddockGap(10.0F);
    if(ui.hdr_mode && SettingsDisclosure("ps5-hdr-tuning","HDR brightness",ui.hdr_tuning,width)){
        bool changed=false;
        PaddockGap(18.0F);
        SettingsHelp("HDR10 preserves the original contrast. Adjust reference white for a comfortable brightness; the game and menus use the same neutral mapping.",width);
        PaddockGap(18.0F);
        const auto nits=[](int v){return SettingsRangeValue{std::to_string(v),"nits"};};
        ImGui::BeginGroup();
        changed|=SettingsRange("ps5-hdr-paper","Menu / reference white",&ui.paper_white,80,300,width,nits,
                               "Comfortable menu brightness. 200 nits is a starting point.");
        ImGui::EndGroup(); PaddockGap(18.0F);
        ImGui::BeginGroup();
        changed|=SettingsRange("ps5-hdr-peak","Display peak brightness",&ui.peak,400,2000,width,nits,
                               "Set this to the calibrated capability of your TV. This is independent from menu brightness.");
        ImGui::EndGroup(); PaddockGap(18.0F);
        SettingsCheck("ps5-hdr-chart","Show HDR brightness pattern",&ui.hdr_pattern,width,
                      "Bottom left: reference white and black. Bottom right: peak and 98% peak. The pattern disappears when this page closes.");
        if(changed)dkrr_ps5_hdr_preview({unsigned(ui.hdr_mode),unsigned(ui.paper_white),unsigned(ui.peak),unsigned(ui.expansion)});
        if(ui.hdr_pattern)dkrr_ps5_hdr_calibration_pattern(1);
    }
    ui.expansion=0;
    PaddockGap(18.0F);
    if (SettingsRaceButton("APPLY TV OUTPUT FOR NEXT START", false)) {
        const unsigned resolution = ui.resolution == 2 ? 2160 : ui.resolution == 1 ? 1440 : 1080;
        const dkrr_ps5_display_choice c{resolution, ui.refresh == 1 ? 120U : 60U};
        const bool saved=dkrr_ps5_choose_output(c,{unsigned(ui.hdr_mode),unsigned(ui.paper_white),unsigned(ui.peak),unsigned(ui.expansion)})==0;
        ui.status = saved
            ? "TV output saved. Close DKR-R and launch it again to apply. Current game and progress stay unchanged."
            : "TV output could not be confirmed saved. Current output is unchanged; reopen this page to check the saved choice.";
    }
    PaddockGap(6.0F);
    if (SettingsRaceButton("RESTORE 1080p / 60 Hz FOR NEXT START", false)) {
        if (dkrr_ps5_choose_output({1080,60},{0,200,800,0}) == 0) {
            ui.resolution = ui.refresh = ui.hdr_mode = 0;ui.hdr_pattern=false;
            dkrr_ps5_hdr_calibration_pattern(0);
            ui.status = "1080p / 60 Hz saved for the next start.";
        } else ui.status = "The default could not be saved. The current output stays unchanged.";
    }
    if (!ui.status.empty()) { PaddockGap(8.0F); SettingsHelp(ui.status, width); }
}
