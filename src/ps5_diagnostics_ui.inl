// SPDX-License-Identifier: GPL-3.0-or-later
// Existing Graphics / Renderer & performance. No separate diagnostic title.
void DrawPs5Diagnostics(float width) {
    static bool save_pending=false;
    static std::string status;
    SettingsHelp(std::string("Build: ")+dkrr_diag_build(),width);
    SettingsHelp(dkrr_diag_journal_ready()?"Crash details are kept in this game's diagnostics folder. The existing performance overlay shows live rates.":"The local diagnostic journal is unavailable. Use the PC log capture; in-memory measurements can still run.",width);
    PaddockGap(8.0F);
    if(SettingsRaceButton("START 60-SECOND FRAME-TIME CAPTURE",false)) {
        status=dkrr_diag_capture_start()==0?"Capture started. Return to the race; it stops after 60 seconds.":"Capture could not start. Try again.";
        save_pending=false;
    }
    const auto c=dkrr_diag_capture_state();
    if(c.active || c.intervals) {
        char text[240];
        const double fps=c.elapsed_ns?double(c.intervals)*1e9/double(c.elapsed_ns):0.0;
        std::snprintf(text,sizeof(text),"%s: %.2f present returns/s, largest gap %.2f ms, gaps over 50 ms: %llu%s.",
            c.active?"Capturing":"Capture",fps,double(c.max_gap_ns)/1e6,
            static_cast<unsigned long long>(c.over_50ms),c.dropped?" (samples dropped; rate incomplete)":"");
        PaddockGap(8.0F);SettingsHelp(text,width);
    }
    PaddockGap(8.0F);
    if(SettingsRaceButton("STOP AND SAVE FRAME-TIME CAPTURE",false))save_pending=true;
    if(save_pending){const int saved=dkrr_diag_capture_finish();
        if(saved==0){save_pending=false;status=c.intervals?"Capture saved in the current diagnostics session.":"Capture stopped; no frame intervals were recorded.";}
        else if(saved==-2){save_pending=false;status="Capture stopped in memory; the local journal could not be written.";}
    }
    SettingsHelp("Measures successful presentation returns, not HDMI scanout or simulation speed. Captures include any time spent in menus. No frame-by-frame file/network logging.",width);
    if(!status.empty()) {PaddockGap(8.0F);SettingsHelp(status,width);}
}
