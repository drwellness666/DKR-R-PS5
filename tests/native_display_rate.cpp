// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_display_options.h"
#include <cassert>
#include <cstdio>
static struct dkrr_ps5_display_status status{};
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status(void) { return status; }
#include "detect_display_rate.inc"
int main() {
    assert(DetectDisplayRate()==60); // native surface not created yet
    status={3840,2160,119880,0,1};assert(DetectDisplayRate()==120);
    status.refresh_millihz=59940;status.fallback=1;assert(DetectDisplayRate()==60);
    status={2560,1440,120000,0,1};assert(DetectDisplayRate()==120);
    status={1920,1080,119880,0,1};assert(DetectDisplayRate()==120);
    for (unsigned rate:{0U,30000U,48000U,60000U,121001U,240000U,500000U}) {
        status.refresh_millihz=rate;assert(DetectDisplayRate()==60);
    }
    status={0,2160,119880,0,1};assert(DetectDisplayRate()==60);
    status={3840,0,119880,0,1};assert(DetectDisplayRate()==60);
    puts("PASS actual PS5 DetectDisplayRate: initial safe baseline, accepted native 119.88/120, 59.94 fallback and invalid status; no SDL dummy query");
}
