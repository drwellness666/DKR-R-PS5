// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 display surface adapter for the pinned Plume Vulkan backend.
// Uses enumerated modes and planes, with a preference for 1080p at <=60 Hz.
#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <vector>

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
        // Prefer 1080p, then the smallest advertised mode; prefer <=60 Hz.
        std::stable_sort(modes.begin(), modes.end(), [](const auto &a, const auto &b) {
            auto score = [](const auto &m) {
                const auto size = m.parameters.visibleRegion;
                const bool preferred = size.width == 1920 && size.height == 1080;
                return uint64_t(!preferred) * (uint64_t(1) << 62) +
                       uint64_t(m.parameters.refreshRate > 61000) * (uint64_t(1) << 61) +
                       uint64_t(size.width) * size.height * 1000000 + m.parameters.refreshRate;
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
                if (result != VK_SUCCESS) return result;
                *extent = size;
                *refreshMilliHz = mode.parameters.refreshRate;
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
