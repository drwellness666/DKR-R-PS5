# SPDX-License-Identifier: GPL-3.0-or-later
if(DKR_NETPLAY_WEBRTC)
    get_target_property(_network_directory datachannel SOURCE_DIR)
    set(_network_stage "${CMAKE_BINARY_DIR}/native-network")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -B
        "${DKR_PS5_ADAPTERS_ROOT}/../tools/stage_ps5_network.py"
        "${_network_directory}" "${_network_stage}" RESULT_VARIABLE _network_result)
    if(NOT _network_result EQUAL 0)
        message(FATAL_ERROR "Pinned RTC wakeup sources changed")
    endif()
    file(READ "${_network_stage}/RECEIPT.json" _network_swaps)
    foreach(_target datachannel juice juice-static)
        get_target_property(_sources ${_target} SOURCES)
        set(_replaced 0)
        foreach(_index RANGE 0 1)
            string(JSON _old GET "${_network_swaps}" ${_index} old)
            string(JSON _new GET "${_network_swaps}" ${_index} new)
            list(FIND _sources "${_old}" _found)
            if(NOT _found EQUAL -1)
                dkrr_release_source_properties("${_old}" "${_new}" ${_target})
                list(TRANSFORM _sources REPLACE "^${_old}$" "${_new}")
                math(EXPR _replaced "${_replaced}+1")
            endif()
        endforeach()
        if(NOT _replaced EQUAL 1)
            message(FATAL_ERROR "Unexpected RTC wakeup target: ${_target}")
        endif()
        set_property(TARGET ${_target} PROPERTY SOURCES "${_sources}")
        target_include_directories(${_target} PRIVATE "${DKR_PS5_ADAPTERS_ROOT}")
        target_compile_options(${_target} PRIVATE
            "SHELL:-include ${DKR_PS5_ADAPTERS_ROOT}/ps5_net_alias.h")
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${DKR_PS5_ADAPTERS_ROOT}/../tools/stage_ps5_network.py"
        "${DKR_PS5_ADAPTERS_ROOT}/ps5_net.h"
        "${DKR_PS5_ADAPTERS_ROOT}/ps5_net_alias.h")
endif()
