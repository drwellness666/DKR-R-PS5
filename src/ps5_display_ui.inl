// SPDX-License-Identifier: GPL-3.0-or-later
// Existing Graphics > Game display card; native replacement for Window mode.
struct Ps5DisplayUiState {
    int resolution = 0, refresh = 0;
    bool loaded = false, hdr_tuning=false, hdr_pattern=false;
    int hdr_mode=0, paper_white=200, peak=800, expansion=35;
    std::string status;
};
Ps5DisplayUiState g_ps5_display_ui;
void DrawPs5DisplayFields(float width, bool entered) {
    auto &ui = g_ps5_display_ui;
    const auto active = dkrr_ps5_display_status();
    if (entered || !ui.loaded) {
        dkrr_ps5_display_choice c{};
        const int rc = dkrr_ps5_read_display_choice(&c);
        ui.resolution = c.resolution == 2160 ? 2 : c.resolution == 1440 ? 1 : 0;
        ui.refresh = c.refresh == 120 && active.high_refresh_build ? 1 : 0;
        dkrr_ps5_hdr_choice hdr{};dkrr_ps5_read_hdr_choice(&hdr);
        ui.hdr_mode=int(hdr.enabled);ui.paper_white=int(hdr.paper_white);ui.peak=int(hdr.peak);ui.expansion=int(hdr.expansion);
        ui.loaded = true;
        ui.status = rc < 0 ? "The saved TV output choice could not be read. The safe default was used." : "";
    }
    SettingsDropdown("ps5-tv-size", "TV output resolution", &ui.resolution,
                     {"1080p (default)", "1440p", "4K (2160p)"}, width, false,
                     "Output buffer size. The game render resolution below controls detail separately. Applies on the next start.");
    PaddockGap(10.0F);
    if (active.high_refresh_build) {
        SettingsDropdown("ps5-tv-rate", "TV refresh rate", &ui.refresh,
                         {"60 Hz (default)", "120 Hz (experimental)"}, width, false,
                         "120 Hz is used only when offered by the console. Match display targets the active output rate after restart. It does not guarantee 120 rendered FPS.");
    } else {
        SettingsHelp("TV refresh rate: 60 Hz. A separate optional test build enables the 120 Hz choice.", width);
    }
    PaddockGap(10.0F);
    SettingsDropdown("ps5-hdr-mode","HDR output",&ui.hdr_mode,
                     {"Off (SDR, default)","HDR10 (experimental)"},width,false,
                     "Maps the original SDR game to 10-bit BT.2020 / PQ. Requires PS5 HDR and a compatible TV. Applies on the next start; unsupported output falls back to SDR.");
    PaddockGap(10.0F);
    if(ui.hdr_mode && SettingsDisclosure("ps5-hdr-tuning","HDR brightness",ui.hdr_tuning,width)){
        bool changed=false;
        const auto nits=[](int v){return SettingsRangeValue{std::to_string(v),"nits"};};
        changed|=SettingsRange("ps5-hdr-paper","Menu / reference white",&ui.paper_white,80,300,width,nits,
                               "Comfortable menu brightness. 200 nits is a starting point.");
        changed|=SettingsRange("ps5-hdr-peak","Display peak brightness",&ui.peak,400,2000,width,nits,
                               "Set this to the calibrated capability of your TV. This is independent from menu brightness.");
        changed|=SettingsRange("ps5-hdr-expansion","Scene highlight expansion",&ui.expansion,0,100,width,
                               [](int v){return SettingsRangeValue{std::to_string(v),"%"};},"0% keeps SDR reference luminance. Larger values expand only bright scene regions; menu text keeps reference white.");
        SettingsCheck("ps5-hdr-chart","Show HDR brightness pattern",&ui.hdr_pattern,width,
                      "Bottom left: reference white and black. Bottom right: peak and 98% peak. The pattern disappears when this page closes.");
        if(changed)dkrr_ps5_hdr_preview({unsigned(ui.hdr_mode),unsigned(ui.paper_white),unsigned(ui.peak),unsigned(ui.expansion)});
        if(ui.hdr_pattern)dkrr_ps5_hdr_calibration_pattern(1);
    }
    if (SettingsRaceButton("APPLY TV OUTPUT FOR NEXT START", false)) {
        const unsigned resolution = ui.resolution == 2 ? 2160 : ui.resolution == 1 ? 1440 : 1080;
        const dkrr_ps5_display_choice c{resolution, ui.refresh == 1 ? 120U : 60U};
        const bool display_saved=dkrr_ps5_choose_display(c)==0;
        const bool hdr_saved=dkrr_ps5_choose_hdr({unsigned(ui.hdr_mode),unsigned(ui.paper_white),unsigned(ui.peak),unsigned(ui.expansion)})==0;
        ui.status = display_saved && hdr_saved
            ? "TV output saved. Close DKR-R and launch it again to apply. Current game and progress stay unchanged."
            : "Some output settings could not be saved. Current output is unchanged; reopen this page to check the saved choices.";
    }
    PaddockGap(6.0F);
    if (SettingsRaceButton("RESTORE 1080p / 60 Hz FOR NEXT START", false)) {
        if (dkrr_ps5_choose_display({1080, 60}) == 0 && dkrr_ps5_choose_hdr({0,200,800,35}) == 0) {
            ui.resolution = ui.refresh = ui.hdr_mode = 0;ui.hdr_pattern=false;
            dkrr_ps5_hdr_calibration_pattern(0);
            ui.status = "1080p / 60 Hz saved for the next start.";
        } else ui.status = "The default could not be saved. The current output stays unchanged.";
    }
    if (active.width && active.height) {
        char text[180];
        std::snprintf(text, sizeof(text), "Output buffer: %u x %u; nominal mode %.2f Hz%s. Actual HDMI / VRR timing follows PS5 and TV settings.",
                      active.width, active.height, double(active.refresh_millihz) / 1000.0,
                      active.fallback ? " (fallback)" : "");
        PaddockGap(10.0F); SettingsHelp(text, width);
    }
    PaddockGap(8.0F);
    const auto hdr_status=dkrr_ps5_hdr_status();
    SettingsHelp(hdr_status.output==2 ? "Colour output: HDR10, HDMI HDR confirmed." :
                 hdr_status.output==1 ? "Colour output: HDR10 buffer; checking HDMI status." :
                 hdr_status.output==3 ? "Colour output: SDR fallback. HDR was unavailable or could not be initialized; saved preference is retained." : "Colour output: SDR.",width);
    SettingsHelp("VRR Automatic is supported by the title declaration. Leave Apply to unsupported games off. ALLM follows PS5 settings. Check actual VRR on the TV.", width);
    if (!ui.status.empty()) { PaddockGap(8.0F); SettingsHelp(ui.status, width); }
}
