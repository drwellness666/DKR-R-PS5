// SPDX-License-Identifier: GPL-3.0-or-later
// Close only this title, after its renderer resources have been released.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <unistd.h>
#ifndef DKR_NATIVE_TITLE_ID
#define DKR_NATIVE_TITLE_ID "PPSA94677"
#endif
struct DkrrAppInfo {
    uint32_t app_id;
    uint64_t unknown1;
    // Public ShadowMount/kstuff/native-tooling layout: title starts at byte 16.
    char title_id[14];
    char unknown2[0x3c];
};
static_assert(offsetof(DkrrAppInfo, title_id) == 16, "Native AppInfo title offset changed");
extern "C" int sceKernelGetAppInfo(pid_t, DkrrAppInfo *);
extern "C" int sceSystemServiceKillApp(int, int, int, int);
static int dkrr_close_this_title() {
    DkrrAppInfo info = {};
    int rc = sceKernelGetAppInfo(getpid(), &info);
    report("own app-info rc=0x%08x app-id=0x%08x title=%.14s",unsigned(rc),info.app_id,info.title_id);
    if (rc != 0 || info.app_id == 0 ||
        std::memcmp(info.title_id, DKR_NATIVE_TITLE_ID, 9) != 0)
        return -1;
    return sceSystemServiceKillApp(static_cast<int>(info.app_id), -1, 0, 0);
}
