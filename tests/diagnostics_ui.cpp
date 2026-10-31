// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_diagnostics.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
static std::string pressed;
static std::vector<std::string> help;
static void SettingsHelp(const std::string& text,float){help.push_back(text);}
static void PaddockGap(float){}
static bool SettingsRaceButton(const char* label,bool){return pressed==label;}
#include "ps5_diagnostics_ui.inl"
static bool contains(const char* needle){for(const auto& line:help)if(line.find(needle)!=std::string::npos)return true;return false;}
int main(){
    DrawPs5Diagnostics(900);assert(!dkrr_diag_capture_state().active);assert(contains("No diagnostic file is available yet"));
    pressed="START 60-SECOND FRAME-TIME CAPTURE";help.clear();DrawPs5Diagnostics(900);assert(dkrr_diag_capture_state().active);assert(contains("Capture started"));
    pressed="STOP AND SAVE FRAME-TIME CAPTURE";help.clear();DrawPs5Diagnostics(900);assert(!dkrr_diag_capture_state().active);assert(contains("journal could not be written"));
    pressed.clear();help.clear();DrawPs5Diagnostics(900);assert(!dkrr_diag_capture_state().active);assert(contains("not HDMI scanout"));
    puts("PASS actual diagnostic UI actions: explicit start/stop, disabled default, honest missing-journal result");
}
