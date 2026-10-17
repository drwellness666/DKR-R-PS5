// SPDX-License-Identifier: GPL-3.0-or-later
// Actual preference adapter, VideoOut gate and Vulkan display adapter.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <sys/stat.h>
#include <vector>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "ps5_display_options.h"
#include "ps5_hdr_options.h"
namespace fs = std::filesystem;
static std::vector<VkDisplayModePropertiesKHR> offered;
static int create_failure = 0, opened = 7, supported_calls = 0, restored = 0;
static bool plane_supported = true;
static VkDisplayKHR display() { return reinterpret_cast<VkDisplayKHR>(uintptr_t(1)); }
static VkDisplayModeKHR handle(unsigned n) { return reinterpret_cast<VkDisplayModeKHR>(uintptr_t(n)); }
static VkResult VKAPI_CALL displays(VkPhysicalDevice, uint32_t *n, VkDisplayPropertiesKHR *out) {
    if (!out) { *n = 1; return VK_SUCCESS; }
    *n = 1; out[0] = {}; out[0].display = display(); out[0].supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    return VK_SUCCESS;
}
static VkResult VKAPI_CALL modes(VkPhysicalDevice, VkDisplayKHR, uint32_t *n, VkDisplayModePropertiesKHR *out) {
    if (!out) { *n = unsigned(offered.size()); return VK_SUCCESS; }
    assert(*n >= offered.size()); std::copy(offered.begin(), offered.end(), out); *n = unsigned(offered.size()); return VK_SUCCESS;
}
static VkResult VKAPI_CALL planes(VkPhysicalDevice, uint32_t *n, VkDisplayPlanePropertiesKHR *out) {
    if (!out) { *n = 1; return VK_SUCCESS; }
    *n = 1; out[0] = {}; return VK_SUCCESS;
}
static VkResult VKAPI_CALL supported(VkPhysicalDevice, uint32_t, uint32_t *n, VkDisplayKHR *out) {
    if (!out) { *n = plane_supported ? 1 : 0; return VK_SUCCESS; }
    *n = 1; out[0] = display(); return VK_SUCCESS;
}
static VkResult VKAPI_CALL caps(VkPhysicalDevice, VkDisplayModeKHR m, uint32_t, VkDisplayPlaneCapabilitiesKHR *out) {
    *out = {}; out->supportedAlpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
    const auto it = std::find_if(offered.begin(), offered.end(), [m](const auto &x) { return x.displayMode == m; }); assert(it != offered.end());
    out->minDstExtent = out->maxDstExtent = out->minSrcExtent = out->maxSrcExtent = it->parameters.visibleRegion;
    return VK_SUCCESS;
}
static VkResult VKAPI_CALL create(VkInstance, const VkDisplaySurfaceCreateInfoKHR *info, const VkAllocationCallbacks *, VkSurfaceKHR *out) {
    if (create_failure == 2) return VK_ERROR_DEVICE_LOST;
    if (create_failure == 1 && info->imageExtent.height == 2160) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out = reinterpret_cast<VkSurfaceKHR>(uintptr_t(2)); return VK_SUCCESS;
}
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR = displays;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR = modes;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR = planes;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR = supported;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR = caps;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR = create;
#include "plume_ps5_display.h"
extern "C" int __real_sceVideoOutOpen(int32_t, int32_t, int32_t, const void *) { return opened; }
extern "C" int __real_sceVideoOutIsOutputSupported(int32_t, uint32_t, const void *, const void *, const void *) { ++supported_calls; return 1; }
extern "C" int sceVideoOutConfigureOutput(int32_t h, uint32_t m, const void *a, const void *b, const void *c) {
    assert(h == 7 && m == 1 && !a && !b && !c); ++restored; return 0;
}
extern "C" int __wrap_sceVideoOutOpen(int32_t, int32_t, int32_t, const void *);
extern "C" int __wrap_sceVideoOutIsOutputSupported(int32_t, uint32_t, const void *, const void *, const void *);
static void write(const fs::path &p, const std::string &s) { std::ofstream(p, std::ios::binary) << s; }
static std::string read(const fs::path &p) { std::ifstream f(p); return {std::istreambuf_iterator<char>(f), {}}; }
// Draw helpers are modeled; the included page and its preference calls are
// actual production code. Visual rendering/navigation remain console checks.
static std::string clicked;
static std::vector<std::string> fields;
static bool SettingsDropdown(const char *, std::string_view label, int *, std::initializer_list<const char *>, float, bool, const char *) {
    fields.emplace_back(label); return false;
}
static void PaddockGap(float) {}
static void SettingsHelp(std::string_view, float) {}
static bool SettingsRaceButton(const char *label, bool) { return clicked == label; }
struct SettingsRangeValue { std::string number, unit; };
static std::string changed_range;
static bool SettingsDisclosure(const char *, std::string_view label, bool &open, float) {
    if (clicked == label) open = !open;
    return open;
}
template<class Format>
static bool SettingsRange(const char *id, std::string_view, int *value, int minimum, int maximum,
                          float, Format &&format, std::string_view = {}, bool = false) {
    const SettingsRangeValue shown = format(*value);
    assert(!shown.number.empty() && !shown.unit.empty());
    if (changed_range != id) return false;
    *value = 210; assert(*value >= minimum && *value <= maximum);
    return true;
}
static bool SettingsCheck(const char *, std::string_view label, bool *value, float,
                          std::string_view = {}, bool = false) {
    if (clicked != label) return false;
    *value = !*value; return true;
}
#include "ps5_display_ui.inl"
static void add(unsigned w, unsigned h, unsigned hz) { offered.push_back({handle(unsigned(offered.size()+1)), {{w,h},hz}}); }
static VkResult surface(unsigned expected_h, unsigned expected_hz) {
    VkSurfaceKHR s{}; VkExtent2D extent{}; uint32_t hz{};
    const auto rc = plume::ps5CreateDisplaySurface(nullptr, nullptr, &s, &extent, &hz);
    if (rc == VK_SUCCESS) { assert(extent.height == expected_h && hz == expected_hz); const auto status = dkrr_ps5_display_status(); assert(status.height == expected_h && status.refresh_millihz == expected_hz); }
    return rc;
}
int main() {
    const fs::path root(DKR_DISPLAY_DATA_ROOT), pref = root / "preferences/display-output.txt", temp = root / "preferences/display-output.txt.next";
    fs::create_directories(pref.parent_path()); write(root / "save-fixture", "preserve progress");
    dkrr_ps5_initialize_hdr();
    dkrr_ps5_display_choice c{}; assert(dkrr_ps5_read_display_choice(&c) == 0 && c.resolution == 1080 && c.refresh == 60);
    assert(dkrr_ps5_read_display_choice(nullptr) == -1);
    for (unsigned res : {1080U,1440U,2160U}) for (unsigned hz : {60U,120U}) {
        const int rc = dkrr_ps5_choose_display({res,hz}); assert(rc == (hz == 120 && !DKR_PS5_HIGH_REFRESH_BUILD ? -1 : 0));
        if (rc) continue;
        assert(dkrr_ps5_read_display_choice(&c) == 1 && c.resolution == res && c.refresh == hz);
        dkrr_ps5_initialize_display(); offered.clear(); add(3840,2160,119880);add(3840,2160,59940);add(2560,1440,119880);add(2560,1440,59940);add(1920,1080,119880);add(1920,1080,59940);
        assert(surface(res,hz == 120 ? 119880 : 59940) == VK_SUCCESS);assert(!dkrr_ps5_display_status().fallback);
        std::reverse(offered.begin(),offered.end());assert(surface(res,hz == 120 ? 119880 : 59940) == VK_SUCCESS);
        // Applying a pending choice never mutates the active process.
        assert(dkrr_ps5_choose_display({1080,60}) == 0);assert(dkrr_ps5_boot_display_choice().resolution == res);
    }
    assert(dkrr_ps5_choose_display({720,60}) == -1 && dkrr_ps5_choose_display({1080,500}) == -1);
    for (const auto &malformed : {"", "resolution=2160p\nrefresh=60\nunknown=1\n", "resolution=2160p\nrefresh=60\nrefresh=120\n", "resolution=1080p\nrefresh=60\n\n", "resolution=1080p\nrefresh=0\n"}) {
        write(pref,malformed); assert(dkrr_ps5_read_display_choice(&c) == -1); dkrr_ps5_initialize_display(); assert(dkrr_ps5_boot_display_choice().resolution == 1080);
        assert(dkrr_ps5_choose_display({1080,60}) == -1);assert(read(pref)==malformed);
    }
    write(pref,std::string(65,'x'));assert(dkrr_ps5_read_display_choice(&c)==-1);
    write(pref,std::string("resolution=1080p\nrefresh=60\n") + std::string(1,'\0')); assert(dkrr_ps5_read_display_choice(&c)==-1);
    fs::remove(pref); fs::create_symlink(root/"save-fixture",pref);assert(dkrr_ps5_choose_display({2160,60})==-1);assert(dkrr_ps5_read_display_choice(&c)==-1);fs::remove(pref);
    write(temp,"interrupted write: preserve");assert(dkrr_ps5_choose_display({1080,60})==0);assert(read(temp)=="interrupted write: preserve");fs::remove(temp);
    assert(dkrr_ps5_choose_display({2160,60})==0);dkrr_ps5_initialize_display();offered.clear();add(1920,1080,59940);assert(surface(1080,59940)==VK_SUCCESS);assert(dkrr_ps5_display_status().fallback);
    offered.clear();add(3840,2160,59940);add(1920,1080,59940);create_failure=1;assert(surface(1080,59940)==VK_SUCCESS);assert(dkrr_ps5_display_status().fallback);
    create_failure=2;assert(surface(0,0)==VK_ERROR_DEVICE_LOST);create_failure=0;
    plane_supported=false;assert(surface(0,0)==VK_ERROR_INITIALIZATION_FAILED);plane_supported=true;
    offered.clear();assert(surface(0,0)==VK_ERROR_INITIALIZATION_FAILED);
    vkCreateDisplayPlaneSurfaceKHR=nullptr;assert(surface(0,0)==VK_ERROR_EXTENSION_NOT_PRESENT);vkCreateDisplayPlaneSurfaceKHR=create;
    assert(dkrr_ps5_choose_display({1080,60})==0);dkrr_ps5_initialize_display();
    assert(__wrap_sceVideoOutIsOutputSupported(7,15,nullptr,nullptr,nullptr)==0 && supported_calls==0);
    assert(__wrap_sceVideoOutIsOutputSupported(7,123,nullptr,nullptr,nullptr)==1 && supported_calls==1);
    assert(__wrap_sceVideoOutOpen(0xff,0,0,nullptr)==7);assert(restored==int(DKR_PS5_HIGH_REFRESH_BUILD));
    opened=-1;assert(__wrap_sceVideoOutOpen(0xff,0,0,nullptr)==-1);assert(restored==int(DKR_PS5_HIGH_REFRESH_BUILD));opened=7;
    if (DKR_PS5_HIGH_REFRESH_BUILD) {
        assert(dkrr_ps5_choose_display({2160,120})==0);dkrr_ps5_initialize_display();assert(__wrap_sceVideoOutIsOutputSupported(7,15,nullptr,nullptr,nullptr)==1 && supported_calls==2);
        assert(__wrap_sceVideoOutOpen(0xff,0,0,nullptr)==7);assert(restored==1);
        offered.clear();add(3840,2160,59940);assert(surface(2160,59940)==VK_SUCCESS && dkrr_ps5_display_status().fallback);
    }
    // Existing card's native fields read once on entry, and only explicit
    // Apply/Reset changes the pending preference. Active mode stays intact.
    const auto boot_before = dkrr_ps5_boot_display_choice();
    DrawPs5DisplayFields(800, true);assert(fields[0]=="TV output resolution");
    assert(fields.size()==size_t(DKR_PS5_HIGH_REFRESH_BUILD ? 3 : 2) && fields.back()=="HDR output");
    const std::string before = read(pref);
    g_ps5_display_ui.resolution = 2; g_ps5_display_ui.refresh = DKR_PS5_HIGH_REFRESH_BUILD ? 1 : 0;
    g_ps5_display_ui.hdr_mode=1; g_ps5_display_ui.paper_white=180; g_ps5_display_ui.peak=700;
    DrawPs5DisplayFields(800, false);assert(read(pref)==before);
    clicked="APPLY TV OUTPUT FOR NEXT START";DrawPs5DisplayFields(800, false);clicked.clear();
    assert(dkrr_ps5_read_display_choice(&c)==1 && c.resolution==2160 && c.refresh==unsigned(DKR_PS5_HIGH_REFRESH_BUILD ? 120 : 60));
    assert(dkrr_ps5_boot_display_choice().resolution==boot_before.resolution && dkrr_ps5_boot_display_choice().refresh==boot_before.refresh);
    dkrr_ps5_hdr_choice hdr{}; assert(dkrr_ps5_read_hdr_choice(&hdr)==1 && hdr.enabled==1 && hdr.paper_white==180 && hdr.peak==700);
    assert(dkrr_ps5_hdr_requested()==0); // Apply changes the next launch only.
    clicked="HDR brightness"; DrawPs5DisplayFields(800,false); clicked.clear();
    assert(g_ps5_display_ui.hdr_tuning);
    changed_range="ps5-hdr-paper"; DrawPs5DisplayFields(800,false); changed_range.clear();
    assert(dkrr_ps5_hdr_calibration().paper_white==210);
    assert(dkrr_ps5_read_hdr_choice(&hdr)==1 && hdr.paper_white==180); // Preview is not a save.
    clicked="Show HDR brightness pattern"; DrawPs5DisplayFields(800,false); clicked.clear();
    assert(dkrr_ps5_hdr_pattern()==1);
    clicked="RESTORE 1080p / 60 Hz FOR NEXT START";DrawPs5DisplayFields(800, false);clicked.clear();
    assert(dkrr_ps5_read_display_choice(&c)==1 && c.resolution==1080 && c.refresh==60);
    assert(g_ps5_display_ui.resolution==0 && g_ps5_display_ui.refresh==0);
    assert(g_ps5_display_ui.hdr_mode==0 && dkrr_ps5_hdr_pattern()==0);
    assert(dkrr_ps5_read_hdr_choice(&hdr)==1 && hdr.enabled==0 && hdr.paper_white==200);
    write(pref,"unrecognized: preserve");DrawPs5DisplayFields(800, true);
    assert(!g_ps5_display_ui.status.empty());clicked="APPLY TV OUTPUT FOR NEXT START";DrawPs5DisplayFields(800, false);clicked.clear();
    assert(read(pref)=="unrecognized: preserve");
    fs::remove(pref);assert(mkfifo(pref.c_str(),0600)==0);assert(dkrr_ps5_read_display_choice(&c)==-1);fs::remove(pref);
    assert(read(root/"save-fixture")=="preserve progress");
    fs::rename(root/"preferences",root/"real-preferences"); fs::create_directory_symlink(root/"real-preferences",root/"preferences");assert(dkrr_ps5_read_display_choice(&c)==-1);assert(dkrr_ps5_choose_display({1080,60})==-1);
    assert(dkrr_ps5_display_score(UINT32_MAX,UINT32_MAX,120000,{1080,60})==UINT64_MAX);
    std::puts("PASS: actual output/HDR preference card, next-launch isolation, brightness preview/reset, Vulkan surfaces, 4K/120 fallback, native VideoOut gate, refusal paths and save preservation");
}
