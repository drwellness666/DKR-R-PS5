# SPDX-License-Identifier: GPL-3.0-or-later
# Compose after all upstream safety/performance stages; never edit dependencies.
set(ps5_hdr_root "${DKR_PS5_ADAPTERS_ROOT}/..")
set(ps5_hdr_input "${CMAKE_BINARY_DIR}/generated/ps5-hdr-input")
file(MAKE_DIRECTORY "${ps5_hdr_input}/src/hle" "${ps5_hdr_input}/src/gui")
configure_file("${DKRPORT_ROOT}/runtime-recomp/ps5-release/renderer/rt64_present_queue.cpp" "${ps5_hdr_input}/src/hle/rt64_present_queue.cpp" COPYONLY)
configure_file("${DKR_RT64_SOURCE}/src/gui/rt64_inspector.cpp" "${ps5_hdr_input}/src/gui/rt64_inspector.cpp" COPYONLY)
file(READ "${ps5_hdr_root}/patches/hdr-stage-manifest.json" ps5_hdr_manifest)
foreach(component IN ITEMS rt64 plume)
    string(JSON ps5_hdr_patch GET "${ps5_hdr_manifest}" "${component}" patch)
    string(JSON ps5_hdr_expected GET "${ps5_hdr_manifest}" "${component}" sha256)
    file(SHA256 "${ps5_hdr_root}/patches/${ps5_hdr_patch}" ps5_hdr_actual)
    if(NOT ps5_hdr_actual STREQUAL ps5_hdr_expected)
        message(FATAL_ERROR "PS5 HDR stage patch hash mismatch")
    endif()
endforeach()
foreach(relative IN ITEMS src/hle/rt64_present_queue.cpp src/gui/rt64_inspector.cpp)
    string(JSON ps5_hdr_expected GET "${ps5_hdr_manifest}" rt64 files "${relative}" original_sha256)
    file(SHA256 "${ps5_hdr_input}/${relative}" ps5_hdr_actual)
    if(NOT ps5_hdr_actual STREQUAL ps5_hdr_expected)
        message(FATAL_ERROR "PS5 HDR input differs from pinned safety/performance composition: ${relative}")
    endif()
endforeach()
string(JSON ps5_hdr_expected GET "${ps5_hdr_manifest}" plume files src/contrib/plume/plume_vulkan.cpp original_sha256)
file(SHA256 "${CMAKE_BINARY_DIR}/generated/enhanced-water/src/contrib/plume/plume_vulkan.cpp" ps5_hdr_actual)
if(NOT ps5_hdr_actual STREQUAL ps5_hdr_expected)
    message(FATAL_ERROR "PS5 HDR Plume input differs from pinned performance composition")
endif()
dkr_stage_patch(rt64 "${ps5_hdr_input}" ps5-hdr-renderer
    "${ps5_hdr_root}/patches/0007-rt64-ps5-hdr-present.patch"
    src/hle/rt64_present_queue.cpp src/gui/rt64_inspector.cpp)
set_source_files_properties("${CMAKE_BINARY_DIR}/generated/ps5-hdr-renderer/src/hle/rt64_present_queue.cpp" TARGET_DIRECTORY rt64
    PROPERTIES INCLUDE_DIRECTORIES "${hle_guard_stage}/src/hle;${DKR_RT64_SOURCE}/src/hle;${DKRPORT_ROOT}/runtime-recomp/src/game;${DKR_PS5_ADAPTERS_ROOT}")
set_source_files_properties("${CMAKE_BINARY_DIR}/generated/ps5-hdr-renderer/src/gui/rt64_inspector.cpp" TARGET_DIRECTORY rt64
    PROPERTIES INCLUDE_DIRECTORIES "${DKR_RT64_SOURCE}/src/gui;${DKRPORT_ROOT}/runtime-recomp/src/game")
target_include_directories(rt64 PRIVATE "${DKR_PS5_ADAPTERS_ROOT}")
dkr_stage_patch(plume "${CMAKE_BINARY_DIR}/generated/enhanced-water" ps5-hdr-plume
    "${ps5_hdr_root}/patches/0008-plume-ps5-hdr-swapchain.patch" src/contrib/plume/plume_vulkan.cpp)
set_source_files_properties("${CMAKE_BINARY_DIR}/generated/ps5-hdr-plume/src/contrib/plume/plume_vulkan.cpp" TARGET_DIRECTORY plume
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/generated/enhanced-water/src/contrib/plume;${DKR_RT64_SOURCE}/src/contrib/plume;${DKR_PS5_ADAPTERS_ROOT};${DKRPORT_ROOT}/runtime-recomp/src/game")
