// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 display surface adapter for the pinned Plume Vulkan backend.
// Uses only enumerated modes and planes. Default: 1080p at <=60 Hz.
#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "ps5_display_options.h"

namespace plume {
static VkResult ps5CreateDisplaySurface(VkInstance instance, VkPhysicalDevice physical,
                                       VkSurfaceKHR *surface, VkExtent2D *extent,
                                       uint32_t *refreshMilliHz) {
    if (!vkGetPhysicalDeviceDisplayPropertiesKHR || !vkGetDisplayModePropertiesKHR ||
        !vkGetPhysicalDeviceDisplayPlanePropertiesKHR || !vkGetDisplayPlaneSupportedDisplaysKHR ||
        !vkGetDisplayPlaneCapabilitiesKHR || !vkCreateDisplayPlaneSurfaceKHR)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    uint32_t count = 0;
    VkResult result = vkGetPhysicalDeviceDisplayPropertiesKHR(physical, &count, nullptr);
    if (result != VK_SUCCESS || count == 0 || count > 64)
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    std::vector<VkDisplayPropertiesKHR> displays(count);
    result = vkGetPhysicalDeviceDisplayPropertiesKHR(physical, &count, displays.data());
    if (result != VK_SUCCESS) return result;
    displays.resize(count);
    count = 0;
    result = vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physical, &count, nullptr);
    if (result != VK_SUCCESS || count == 0 || count > 64)
        return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
    std::vector<VkDisplayPlanePropertiesKHR> planes(count);
    result = vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physical, &count, planes.data());
    if (result != VK_SUCCESS) return result;
    planes.resize(count);
    for (const auto &display : displays) {
        if (!(display.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) continue;
        count = 0;
        result = vkGetDisplayModePropertiesKHR(physical, display.display, &count, nullptr);
        if (result != VK_SUCCESS) return result;
        if (count == 0 || count > 128) continue;
        std::vector<VkDisplayModePropertiesKHR> modes(count);
        result = vkGetDisplayModePropertiesKHR(physical, display.display, &count, modes.data());
        if (result != VK_SUCCESS) return result;
        modes.resize(count);
#if defined(DKR_PS5_GAME_DISPLAY)
        const auto choice = dkrr_ps5_boot_display_choice();
#else
        // Standalone GPU/RT64 probes retain their original fixed output and
        // must not gain unresolved imports from the game's preference module.
        const dkrr_ps5_display_choice choice{1080, 60};
#endif
        std::stable_sort(modes.begin(), modes.end(), [choice](const auto &a, const auto &b) {
            const auto score = [choice](const auto &m) {
                return dkrr_ps5_display_score(m.parameters.visibleRegion.width,
                    m.parameters.visibleRegion.height, m.parameters.refreshRate, choice);
            };
            return score(a) < score(b);
        });
        for (uint32_t plane = 0; plane < planes.size(); ++plane) {
            count = 0;
            result = vkGetDisplayPlaneSupportedDisplaysKHR(physical, plane, &count, nullptr);
            if (result != VK_SUCCESS) return result;
            if (count == 0 || count > 64) continue;
            std::vector<VkDisplayKHR> supported(count);
            result = vkGetDisplayPlaneSupportedDisplaysKHR(physical, plane, &count, supported.data());
            if (result != VK_SUCCESS) return result;
            supported.resize(count);
            if (std::find(supported.begin(), supported.end(), display.display) == supported.end()) continue;
            for (const auto &mode : modes) {
                const VkExtent2D size = mode.parameters.visibleRegion;
                VkDisplayPlaneCapabilitiesKHR caps = {};
                result = vkGetDisplayPlaneCapabilitiesKHR(physical, mode.displayMode, plane, &caps);
                if (result != VK_SUCCESS) return result;
                if (!(caps.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR) ||
                    size.width < caps.minDstExtent.width || size.width > caps.maxDstExtent.width ||
                    size.height < caps.minDstExtent.height || size.height > caps.maxDstExtent.height ||
                    size.width < caps.minSrcExtent.width || size.width > caps.maxSrcExtent.width ||
                    size.height < caps.minSrcExtent.height || size.height > caps.maxSrcExtent.height ||
                    caps.minSrcPosition.x > 0 || caps.maxSrcPosition.x < 0 ||
                    caps.minSrcPosition.y > 0 || caps.maxSrcPosition.y < 0 ||
                    caps.minDstPosition.x > 0 || caps.maxDstPosition.x < 0 ||
                    caps.minDstPosition.y > 0 || caps.maxDstPosition.y < 0) continue;
                VkDisplaySurfaceCreateInfoKHR info = {};
                info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
                info.displayMode = mode.displayMode;
                info.planeIndex = plane;
                info.planeStackIndex = planes[plane].currentStackIndex;
                info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                info.globalAlpha = 1.0f;
                info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
                info.imageExtent = size;
                result = vkCreateDisplayPlaneSurfaceKHR(instance, &info, nullptr, surface);
                if (result != VK_SUCCESS) {
                    // A refused optional mode may fall back to another reported
                    // mode. Do not continue after a lost device.
                    if (result == VK_ERROR_DEVICE_LOST ||
                        (choice.resolution == 1080 && choice.refresh == 60)) return result;
                    continue;
                }
                *extent = size;
                *refreshMilliHz = mode.parameters.refreshRate;
#if defined(DKR_PS5_GAME_DISPLAY)
                dkrr_ps5_report_display(size.width, size.height, *refreshMilliHz);
#endif
                std::printf("ps5-plume: display=%ux%u refresh-millihz=%u plane=%u\n",
                            size.width, size.height, *refreshMilliHz, plane);
                std::fflush(stdout);
                return VK_SUCCESS;
            }
        }
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}
}
