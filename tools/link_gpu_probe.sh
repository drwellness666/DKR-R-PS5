#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: link_gpu_probe.sh ABSOLUTE_GPU_WORK' >&2; exit 2; }
work=$1
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
support="$work/PS5_Vulkan"
sdk="$support/.deps/native/ps5-payload-sdk"
plume="$work/Plume"
probe="$work/probe"
archive="$work/radv-build/src/amd/vulkan/libvulkan_radeon.a"
mkdir -p "$probe/gen" "$probe/stubs"
incs=(-I "$plume" -I "$plume/contrib/volk" -I "$plume/contrib/Vulkan-Headers/include" -I "$plume/contrib/VulkanMemoryAllocator/include")
for stage in vert frag; do
    glslangValidator -V --target-env vulkan1.2 --vn "plume_probe_$stage" \
        "$root/src/plume_probe.$stage" -o "$probe/gen/plume_probe_$stage.h"
done
"$sdk/bin/prospero-clang++" -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
    "${incs[@]}" -c "$plume/plume_vulkan.cpp" -o "$probe/plume_vulkan.o"
"$sdk/bin/prospero-clang++" -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    "${incs[@]}" -I "$probe/gen" -c "$root/src/plume_probe.cpp" -o "$probe/plume_probe.o"
for library in libSceAgc libSceAgcDriver; do
    if [[ $library == libSceAgc ]]; then file=agc_canary_link_stub.c; else file=agc_driver_canary_link_stub.c; fi
    "$sdk/bin/prospero-clang" -O2 -fPIC -c "$support/vendor/ps5/sdk/stubs/$file" -o "$probe/stubs/$library.o"
    "$sdk/bin/prospero-lld" --shared -soname "$library.sprx" -o "$probe/stubs/$library.so" "$probe/stubs/$library.o"
done
source "$support/tools/radv-link.sh"
radv_link_recipe "$support" "$sdk" "$archive"
# Payload CRT and its memory layout, plus the unwind boundaries needed by ACO.
cat "$sdk/ldscripts/elf_x86_64.x" "$root/src/ps5_payload_unwind.ld" > "$probe/payload-unwind.ld"
link_flags=(-Wl,-T,"$probe/payload-unwind.ld" -Wl,--gc-sections)
for flag in "${radv_link_flags[@]}"; do link_flags+=("-Wl,$flag"); done
link_inputs=()
for input in "${radv_link_inputs[@]}"; do
    if [[ $input == --* ]]; then link_inputs+=("-Wl,$input"); else link_inputs+=("$input"); fi
done
"$sdk/bin/prospero-clang++" -nodefaultlibs -o "$probe/dkrr-ps5-plume-probe.elf" \
    "$probe/plume_probe.o" "$probe/plume_vulkan.o" "$probe/stubs/libSceAgc.so" "$probe/stubs/libSceAgcDriver.so" \
    "${link_flags[@]}" "${link_inputs[@]}" -lc -lkernel_web -lSceLibcInternal -lSceNet -lSceVideoOut
"$sdk/bin/prospero-strip" --strip-debug "$probe/dkrr-ps5-plume-probe.elf"
echo "Built native Plume/RADV probe: $probe/dkrr-ps5-plume-probe.elf"
