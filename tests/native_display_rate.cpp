// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_display_options.h"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include "presentation_policy.hpp"
static struct dkrr_ps5_display_status status{};
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status(void) { return status; }
#include "detect_display_rate.inc"
namespace RT64 { struct ApplicationWindow { unsigned refreshRate=60; uint32_t getRefreshRate() const; };
#include "native_window_rate.inc"
}

namespace ultramodern::renderer { enum class RefreshRate { Original,Display,Manual }; }
using ultramodern::renderer::RefreshRate;
#include "native_rate_defaults.inc"
namespace RT64 { struct UserConfiguration {
    enum class RefreshRate { Original,Manual };
}; }
static bool modern=true;
namespace dkr::runtime::enhancements { bool modern_presentation_enabled(){return modern;} }
static bool ExperimentalInterpolationEnabled(){return modern;}
static int g_requested_refresh_target,g_effective_refresh_target,g_detected_display_rate;
struct Config { RefreshRate rr_option;int rr_manual_value; };
struct Application { struct {
    RT64::UserConfiguration::RefreshRate refreshRate;
    int refreshRateTarget;
} userConfig; };
static void ApplyRate(Application &application,const Config &config) {
#include "apply_native_rate.inc"
}
int main() {
    assert(g_modern_refresh_mode==RefreshRate::Display);
    assert(g_modern_refresh_target==60); // remembered advanced value, inactive by default
    assert(DetectDisplayRate()==60); // native surface not created yet
    status={3840,2160,119880,0,1};assert(DetectDisplayRate()==120);
    RT64::ApplicationWindow window;assert(window.getRefreshRate()==120);
    assert(std::min(window.getRefreshRate(),120U)==120);
    status.refresh_millihz=59940;status.fallback=1;assert(DetectDisplayRate()==60);assert(window.getRefreshRate()==60);
    status={2560,1440,120000,0,1};assert(DetectDisplayRate()==120);
    status={1920,1080,119880,0,1};assert(DetectDisplayRate()==120);
    for (unsigned rate:{0U,30000U,48000U,60000U,121001U,240000U,500000U}) {
        status.refresh_millihz=rate;assert(DetectDisplayRate()==60);
    }
    status={0,2160,119880,0,1};assert(DetectDisplayRate()==60);
    status={3840,0,119880,0,1};assert(DetectDisplayRate()==60);
    Application app{};
    status={3840,2160,119880,0,1};
    ApplyRate(app,{RefreshRate::Display,30});assert(app.userConfig.refreshRateTarget==120);
    status.refresh_millihz=59940;status.fallback=1;
    ApplyRate(app,{RefreshRate::Display,120});assert(app.userConfig.refreshRateTarget==60);
    status.refresh_millihz=119880;
    ApplyRate(app,{RefreshRate::Manual,60});assert(app.userConfig.refreshRateTarget==120);
    ApplyRate(app,{RefreshRate::Manual,900});assert(app.userConfig.refreshRateTarget==120);
    status.refresh_millihz=59940;status.fallback=1;
    ApplyRate(app,{RefreshRate::Manual,120});assert(app.userConfig.refreshRateTarget==60);
    modern=false;ApplyRate(app,{RefreshRate::Display,120});
    assert(app.userConfig.refreshRate==RT64::UserConfiguration::RefreshRate::Original&&app.userConfig.refreshRateTarget==30);
    puts("PASS actual PS5 default and renderer rate path: Match display targets native 120/60 including fallback; stale manual settings cannot impose a second PS5 cap; Accurate stays 30; no SDL dummy query");
}
