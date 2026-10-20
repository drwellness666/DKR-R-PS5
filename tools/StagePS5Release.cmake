# SPDX-License-Identifier: GPL-3.0-or-later
# Source-only 1.0.7 layers have already been byte-verified and composed by
# prepare_game_sources.py. This is their native PS5 target graph.
get_filename_component(_release_game "${DKRPORT_ROOT}/runtime-recomp/src/game" ABSOLUTE)
set(_camera_native "${DKRPORT_ROOT}/patches/camera-plus/native")
set(_release_headers "${DKRPORT_ROOT}/runtime-recomp/include")
# Both pinned RT64 and N64ModernRuntime ship the same JSON header. SDK 2 uses
# its standard nlohmann include spelling; expose that exact existing header.
set(_release_json "${CMAKE_BINARY_DIR}/generated/ps5-release-json")
file(MAKE_DIRECTORY "${_release_json}/nlohmann")
configure_file("${DKR_MODERN_RUNTIME_SOURCE}/thirdparty/json/json.hpp"
    "${_release_json}/nlohmann/json.hpp" COPYONLY)

target_sources(DKRCustomMagicCodes PRIVATE "${_release_game}/mods/sdk2_bridge.cpp")
target_include_directories(DKRCustomMagicCodes PUBLIC "${_release_headers}" "${_release_json}")
target_sources(DKRPortGame PRIVATE
    "${_release_game}/mod_store.cpp"
    "${_release_game}/mod_store_names.cpp"
    "${_release_game}/mod_store_install.cpp")
target_include_directories(DKRPortGame PRIVATE "${_release_json}" "${_release_headers}" "${_release_game}")

add_library(DKRCameraPresentation STATIC
    "${_release_game}/camera_presentation.cpp" "${_release_game}/camera_scene.cpp"
    "${_release_game}/camera_snapshot.cpp" "${_release_game}/camera_settings.cpp"
    "${_release_game}/camera_actions.cpp" "${_release_game}/camera_session.cpp"
    "${_release_game}/camera_runtime.cpp" "${_release_game}/camera_rt64.cpp")
target_compile_features(DKRCameraPresentation PUBLIC cxx_std_20)
target_compile_options(DKRCameraPresentation PRIVATE -ffp-contract=off -fno-fast-math)
target_include_directories(DKRCameraPresentation PUBLIC "${_release_headers}" "${_release_game}")
target_include_directories(DKRCameraPresentation PRIVATE "${DKR_RT64_SOURCE}/src/contrib")
target_link_libraries(DKRCameraPresentation PUBLIC DKRCustomMagicCodes)
foreach(_revision 77 80)
    set(_kernel "${DKR_CAMERA_KERNEL_V${_revision}}")
    if(NOT EXISTS "${_kernel}/PS5-KERNEL-RECEIPT.json")
        message(FATAL_ERROR "Generate the local PS5 Camera++ draw kernel for ${_revision} first")
    endif()
    add_library(DKRCameraSceneV${_revision} STATIC
        "${_kernel}/camera_scene_functions.c" "${_kernel}/camera_scene_hooks.c"
        "${_kernel}/camera_shadow_kernel.c" "${_camera_native}/camera_kernel.c")
    target_compile_features(DKRCameraSceneV${_revision} PRIVATE c_std_11)
    target_compile_options(DKRCameraSceneV${_revision} PRIVATE -fno-strict-aliasing -ffp-contract=off -fno-fast-math -frounding-math)
    target_include_directories(DKRCameraSceneV${_revision} BEFORE PRIVATE
        "${_kernel}" "${_camera_native}" "${_release_game}")
    foreach(_helper access step fault)
        target_compile_definitions(DKRCameraSceneV${_revision} PRIVATE dkr_camera_${_helper}=dkr_camera_v${_revision}_${_helper})
    endforeach()
    target_compile_definitions(DKRCameraSceneV${_revision} PRIVATE DKR_SHADOW_ENTRY=dkr_camera_v${_revision}_shadow_draw)
    target_link_libraries(DKRCameraPresentation PRIVATE DKRCameraSceneV${_revision})
endforeach()
target_link_libraries(DKRPortGame PRIVATE DKRCameraPresentation)

# Keep the original release renderer bodies with the PS5 Plume/HDR adapter.
file(GLOB _release_renderer_headers "${DKRPORT_ROOT}/runtime-recomp/ps5-release/renderer/*.h"
    "${DKRPORT_ROOT}/runtime-recomp/ps5-release/renderer/*.hpp")
foreach(_header IN LISTS _release_renderer_headers)
    get_filename_component(_name "${_header}" NAME)
    configure_file("${_header}" "${hle_guard_stage}/src/render/${_name}" COPYONLY)
endforeach()

get_target_property(_release_renderer_sources rt64 SOURCES)
get_target_property(_release_renderer_directory rt64 SOURCE_DIR)
set(_release_renderer_result)
foreach(_source IN LISTS _release_renderer_sources)
    if(_source MATCHES "/(rt64_texture_cache|rt64_present_queue|rt64_framebuffer_renderer)\\.cpp$")
        get_filename_component(_name "${_source}" NAME)
        if(_name STREQUAL "rt64_present_queue.cpp")
            set(_kind hle)
        else()
            set(_kind render)
        endif()
        set(_next "${hle_guard_stage}/src/${_kind}/${_name}")
        configure_file("${DKRPORT_ROOT}/runtime-recomp/ps5-release/renderer/${_name}" "${_next}" COPYONLY)
        foreach(_property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS)
            get_property(_defined SOURCE "${_source}" DIRECTORY "${_release_renderer_directory}" PROPERTY ${_property} SET)
            if(_defined)
                get_property(_value SOURCE "${_source}" DIRECTORY "${_release_renderer_directory}" PROPERTY ${_property})
                set_property(SOURCE "${_next}" DIRECTORY "${_release_renderer_directory}" PROPERTY ${_property} "${_value}")
            endif()
        endforeach()
        # All quoted class-layout consumers must share the final guarded
        # header tree; mixing dependency and staged copies redeclares types.
        set_property(SOURCE "${_next}" DIRECTORY "${_release_renderer_directory}" PROPERTY INCLUDE_DIRECTORIES
            "${hle_guard_stage}/src/${_kind}" "${hle_guard_stage}/src" "${hle_guard_stage}/include"
            "${_release_game}" "${DKR_PS5_ADAPTERS_ROOT}")
        list(APPEND _release_renderer_result "${_next}")
    else()
        list(APPEND _release_renderer_result "${_source}")
    endif()
endforeach()
set_property(TARGET rt64 PROPERTY SOURCES "${_release_renderer_result}")

include("${CMAKE_CURRENT_LIST_DIR}/StagePS5HDR.cmake")
