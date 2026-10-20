// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_hdr_policy.hpp"
#include "ps5_output_preferences.hpp"
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_DISPLAY_DATA_ROOT
#define DKR_DISPLAY_DATA_ROOT "/data/dkrr"
#endif
namespace {
// Render snapshots never wait on preference reads, writes or fsync. One word
// publishes a complete validated choice, so readers cannot see mixed fields.
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
constexpr std::uint32_t pack(dkrr_ps5_hdr_choice c){return c.enabled|(c.paper_white<<1)|(c.peak<<10)|(c.expansion<<21);}
constexpr dkrr_ps5_hdr_choice unpack(std::uint32_t c){return {c&1,(c>>1)&511,(c>>10)&2047,(c>>21)&127};}
std::atomic<std::uint32_t> calibration{pack({0,200,800,0})};
std::atomic<unsigned> requested{0},output{0},reason{0},pattern{0};
std::atomic<std::int64_t> pattern_until{0};
std::int64_t now_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
extern "C" int dkrr_ps5_read_hdr_choice(dkrr_ps5_hdr_choice *c){
    if(!c){errno=EINVAL;return -1;}
    dkrr::output_preferences::Choice choice;const int rc=dkrr::output_preferences::read(choice);
    *c=choice.hdr;return rc;
}
extern "C" void dkrr_ps5_initialize_hdr(){dkrr_ps5_hdr_choice c{};dkrr_ps5_read_hdr_choice(&c);calibration.store(pack(c),std::memory_order_release);requested=c.enabled;output=reason=pattern=0;}
extern "C" unsigned dkrr_ps5_hdr_requested(){return requested.load();}
extern "C" dkrr_ps5_hdr_choice dkrr_ps5_hdr_calibration(){return unpack(calibration.load(std::memory_order_acquire));}
extern "C" struct dkrr_ps5_hdr_status dkrr_ps5_hdr_status(){return {output.load(),reason.load()};}
extern "C" void dkrr_ps5_hdr_report(unsigned o,unsigned r){reason=r;output=o;}
extern "C" void dkrr_ps5_hdr_preview(dkrr_ps5_hdr_choice c){if(dkrr::hdr::valid(c))calibration.store(pack(c),std::memory_order_release);}
extern "C" void dkrr_ps5_hdr_calibration_pattern(unsigned p){pattern_until=p?now_ms()+300:0;pattern=p!=0;}
extern "C" unsigned dkrr_ps5_hdr_pattern(){return pattern.load() && now_ms()<pattern_until.load();}
extern "C" int dkrr_ps5_choose_hdr(dkrr_ps5_hdr_choice c){
    const int rc=dkrr::output_preferences::save(nullptr,&c);
    if(!rc)calibration.store(pack(c),std::memory_order_release);
    return rc;
}
