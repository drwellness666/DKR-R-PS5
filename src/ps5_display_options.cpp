// SPDX-License-Identifier: GPL-3.0-or-later
// Own, next-launch display preference. No system settings or game saves.
#include "ps5_display_options.h"
#include "ps5_output_preferences.hpp"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <initializer_list>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_DISPLAY_DATA_ROOT
#define DKR_DISPLAY_DATA_ROOT "/data/dkrr"
#endif
#ifndef DKR_PS5_HIGH_REFRESH_BUILD
#define DKR_PS5_HIGH_REFRESH_BUILD 0
#endif
namespace {
std::atomic<unsigned> boot_resolution{1080}, boot_refresh{60};
std::atomic<unsigned> active_width{0}, active_height{0}, active_refresh{0}, active_fallback{0};
int read_choice(dkrr_ps5_display_choice *choice) {
    if(!choice){errno=EINVAL;return -1;}
    dkrr::output_preferences::Choice c;const int rc=dkrr::output_preferences::read(c);
    *choice=c.display;return rc;
}
}
extern "C" int dkrr_ps5_read_display_choice(dkrr_ps5_display_choice *choice) { return read_choice(choice); }
extern "C" int dkrr_ps5_read_output(dkrr_ps5_display_choice *display,dkrr_ps5_hdr_choice *hdr) {
    if(!display||!hdr){errno=EINVAL;return -1;}
    dkrr::output_preferences::Choice c;const int rc=dkrr::output_preferences::read(c);
    *display=c.display;*hdr=c.hdr;return rc;
}
extern "C" void dkrr_ps5_initialize_display() {
    dkrr_ps5_display_choice c{1080, 60}; read_choice(&c);
    boot_resolution.store(c.resolution, std::memory_order_relaxed);
    boot_refresh.store(DKR_PS5_HIGH_REFRESH_BUILD ? c.refresh : 60, std::memory_order_relaxed);
}
extern "C" dkrr_ps5_display_choice dkrr_ps5_boot_display_choice() {
    return {boot_resolution.load(std::memory_order_relaxed), boot_refresh.load(std::memory_order_relaxed)};
}
extern "C" void dkrr_ps5_report_display(unsigned width, unsigned height, unsigned refresh) {
    const auto c = dkrr_ps5_boot_display_choice();
    active_width.store(width, std::memory_order_relaxed); active_height.store(height, std::memory_order_relaxed);
    active_refresh.store(refresh, std::memory_order_relaxed);
    active_fallback.store(height != c.resolution || (refresh + 500) / 1000 != c.refresh, std::memory_order_relaxed);
}
extern "C" struct dkrr_ps5_display_status dkrr_ps5_display_status() {
    return {active_width.load(std::memory_order_relaxed), active_height.load(std::memory_order_relaxed),
            active_refresh.load(std::memory_order_relaxed), active_fallback.load(std::memory_order_relaxed),
            DKR_PS5_HIGH_REFRESH_BUILD};
}
extern "C" int dkrr_ps5_choose_display(dkrr_ps5_display_choice choice) {
    if(choice.refresh==120&&!DKR_PS5_HIGH_REFRESH_BUILD){errno=EINVAL;return -1;}
    return dkrr::output_preferences::save(&choice,nullptr);
}
extern "C" int dkrr_ps5_choose_output(dkrr_ps5_display_choice display,dkrr_ps5_hdr_choice hdr) {
    hdr.expansion=0;
    if(display.refresh==120&&!DKR_PS5_HIGH_REFRESH_BUILD){errno=EINVAL;return -1;}
    const int rc=dkrr::output_preferences::save(&display,&hdr);
    if(!rc)dkrr_ps5_hdr_preview(hdr);
    return rc;
}
