// SPDX-License-Identifier: GPL-3.0-or-later
// Existing Graphics > Game display card; native replacement for Window mode.
struct Ps5DisplayUiState {
    int resolution = 0, refresh = 0;
    bool loaded = false;
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
                         "120 Hz is used only when offered by the console. Match display below follows the selected rate.");
    } else {
        SettingsHelp("TV refresh rate: 60 Hz. A separate optional test build enables the 120 Hz choice.", width);
    }
    PaddockGap(10.0F);
    if (SettingsRaceButton("APPLY TV OUTPUT FOR NEXT START", false)) {
        const unsigned resolution = ui.resolution == 2 ? 2160 : ui.resolution == 1 ? 1440 : 1080;
        const dkrr_ps5_display_choice c{resolution, ui.refresh == 1 ? 120U : 60U};
        ui.status = dkrr_ps5_choose_display(c) == 0
            ? "TV output saved. Close DKR-R and launch it again to apply. Current game and progress stay unchanged."
            : "TV output could not be saved. The current output and progress stay unchanged.";
    }
    PaddockGap(6.0F);
    if (SettingsRaceButton("RESTORE 1080p / 60 Hz FOR NEXT START", false)) {
        if (dkrr_ps5_choose_display({1080, 60}) == 0) {
            ui.resolution = ui.refresh = 0;
            ui.status = "1080p / 60 Hz saved for the next start.";
        } else ui.status = "The default could not be saved. The current output stays unchanged.";
    }
    if (active.width && active.height) {
        char text[180];
        std::snprintf(text, sizeof(text), "Current output buffer: %u x %u at %.2f Hz%s. HDMI resolution follows PS5 settings.",
                      active.width, active.height, double(active.refresh_millihz) / 1000.0,
                      active.fallback ? " (fallback)" : "");
        PaddockGap(10.0F); SettingsHelp(text, width);
    }
    PaddockGap(8.0F);
    SettingsHelp("Colour output: SDR. HDR is not available in this renderer. VRR and ALLM are controlled in PS5 Screen and Video settings; their actual TV status must be checked on the TV.", width);
    if (!ui.status.empty()) { PaddockGap(8.0F); SettingsHelp(ui.status, width); }
}
