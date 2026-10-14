// SPDX-License-Identifier: GPL-3.0-or-later
// Host-only mode/plane selection tests. Do not create a real Vulkan instance.
#define VOLK_IMPLEMENTATION
#include <volk.h>
#include "plume_ps5_display.h"
#include <cassert>
#include <cstdio>

static VkDisplayKHR display = reinterpret_cast<VkDisplayKHR>(uintptr_t(1));
static bool has1080 = true, opaque = true;
static VkResult capsResult = VK_SUCCESS;
static VkDisplaySurfaceCreateInfoKHR created;
static VkResult displays(VkPhysicalDevice, uint32_t *n, VkDisplayPropertiesKHR *p) {
    *n = 1;
    if (p) { *p = {}; p->display = display; p->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; }
    return VK_SUCCESS;
}
static VkResult planes(VkPhysicalDevice, uint32_t *n, VkDisplayPlanePropertiesKHR *p) {
    *n = 2;
    if (p) { p[0] = {}; p[1] = {}; p[1].currentStackIndex = 3; }
    return VK_SUCCESS;
}
static VkResult supported(VkPhysicalDevice, uint32_t plane, uint32_t *n, VkDisplayKHR *p) {
    *n = plane == 1 ? 1 : 0; if (p && *n) p[0] = display; return VK_SUCCESS;
}
static VkResult modes(VkPhysicalDevice, VkDisplayKHR, uint32_t *n, VkDisplayModePropertiesKHR *p) {
    *n = has1080 ? 3 : 1;
    if (p) {
        p[0] = {reinterpret_cast<VkDisplayModeKHR>(uintptr_t(10)), {{3840,2160}, 59940}};
        if (has1080) {
            p[1] = {reinterpret_cast<VkDisplayModeKHR>(uintptr_t(11)), {{1920,1080}, 119880}};
            p[2] = {reinterpret_cast<VkDisplayModeKHR>(uintptr_t(12)), {{1920,1080}, 59940}};
        }
    }
    return VK_SUCCESS;
}
static VkResult capabilities(VkPhysicalDevice, VkDisplayModeKHR, uint32_t, VkDisplayPlaneCapabilitiesKHR *p) {
    *p = {}; p->supportedAlpha = opaque ? VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR : 0;
    p->maxSrcExtent = p->maxDstExtent = {3840,2160}; return capsResult;
}
static VkResult create(VkInstance, const VkDisplaySurfaceCreateInfoKHR *p, const VkAllocationCallbacks *, VkSurfaceKHR *out) {
    created = *p; *out = reinterpret_cast<VkSurfaceKHR>(uintptr_t(100)); return VK_SUCCESS;
}
int main() {
    vkGetPhysicalDeviceDisplayPropertiesKHR = displays;
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR = planes;
    vkGetDisplayPlaneSupportedDisplaysKHR = supported;
    vkGetDisplayModePropertiesKHR = modes;
    vkGetDisplayPlaneCapabilitiesKHR = capabilities;
    vkCreateDisplayPlaneSurfaceKHR = create;
    VkSurfaceKHR surface = VK_NULL_HANDLE; VkExtent2D size = {}; uint32_t hz = 0;
    auto run = [&]() { surface = VK_NULL_HANDLE; return plume::ps5CreateDisplaySurface(VK_NULL_HANDLE, VK_NULL_HANDLE, &surface, &size, &hz); };
    assert(run() == VK_SUCCESS && size.width == 1920 && size.height == 1080 && hz == 59940);
    assert(created.planeIndex == 1 && created.planeStackIndex == 3);
    has1080 = false;
    assert(run() == VK_SUCCESS && size.width == 3840 && size.height == 2160);
    opaque = false;
    assert(run() == VK_ERROR_INITIALIZATION_FAILED && surface == VK_NULL_HANDLE);
    opaque = true; capsResult = VK_ERROR_DEVICE_LOST;
    assert(run() == VK_ERROR_DEVICE_LOST && surface == VK_NULL_HANDLE);
    vkGetDisplayModePropertiesKHR = nullptr;
    assert(run() == VK_ERROR_EXTENSION_NOT_PRESENT && surface == VK_NULL_HANDLE);
    std::puts("PASS: 1080p/60 preference, supported plane, fallback, alpha rejection, error propagation, missing resolver");
}
