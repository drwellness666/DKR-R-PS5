#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Native FSELF diagnostic; no payload CRT, ROM, Sony binary or retail package.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build_native_gpu_title.sh ABSOLUTE_GPU_WORK' >&2; exit 2; }
work=$1
[[ $work == /* ]] || exit 2
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
support="$work/PS5_Vulkan"
sdk="$support/.deps/native/ps5-payload-sdk"
native="$support/tooling/native"
plume="$work/Plume"
build="$work/native-title"
archive="$work/radv-build/src/amd/vulkan/libvulkan_radeon.a"
probe_source="$root/src/plume_probe.cpp"
extra_incs=()
extra_inputs=()
build_version=5
if [[ ${DKR_RT64_SMOKE:-0} == 1 ]]; then
    rt="${DKR_RT64_WORK:-$root/work/rt64}/rt64"
    rb="${DKR_RT64_WORK:-$root/work/rt64}/build"
    sdl_work="${DKR_SDL_WORK:-$root/work/lowmem}"
    probe_source="$root/src/rt64_probe.cpp"
    build_version=9
    extra_incs=(-DHLSL_CPU -DFFX_GCC -isystem "$rt/src" -isystem "$rt/src/contrib" -isystem "$rt/src/contrib/imgui" -isystem "$rt/src/contrib/hlslpp/include" -isystem "$sdl_work/src/ps5-SDL/include")
    extra_inputs=(--start-group "$rb/rt64.a" "$rb/src/contrib/re-spirv/libre-spirv.a" "$rb/src/contrib/zstd/build/cmake/lib/libzstd.a" "$sdl_work/build/sdl/libSDL2.a" "$sdl_work/build/samplerate/src/libsamplerate.a" --end-group)
fi
mkdir -p "$build/obj" "$build/stubs" "$build/gen"
zlib="$build/zlib-1.3.2"
printf '%s  %s\n' bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16 "$root/vendor/zlib-1.3.2.tar.gz" | sha256sum -c -
if [[ ! -f $zlib/libz.a ]]; then
    mkdir -p "$zlib"
    tar -xzf "$root/vendor/zlib-1.3.2.tar.gz" -C "$zlib" --strip-components=1
    (cd "$zlib" && CC=clang AR=llvm-ar RANLIB=llvm-ranlib ./configure --static && make -j "${DKR_GPU_JOBS:-4}")
fi
clang++ -std=c++20 -O2 -Wall -Wextra -Werror -I "$zlib" \
    "$native/native_app_builder.cpp" "$native/self_container.cpp" \
    "$native/elf_object.cpp" "$native/sce_module_writer.cpp" \
    "$zlib/libz.a" -o "$build/ps5-native-tool"
clang++ -std=c++20 -O2 -Wall -Wextra -Werror "$native/libc_builder.cpp" -o "$build/libc-builder"
for copy in a b; do
    "$build/libc-builder" "$native/runtime/api-surface.txt" "$native/runtime/imports.txt" "$build/libc-$copy.raw.elf"
done
cmp "$build/libc-a.raw.elf" "$build/libc-b.raw.elf"
printf '%s  %s\n' 8ee6e124993e1af26420cb455890fd002f5d6c7e78883c860ce45734e7d002bb "$build/libc-a.raw.elf" | sha256sum -c -
"$build/ps5-native-tool" self --sign --in "$build/libc-a.raw.elf" --out "$build/libc.prx"
printf '%s  %s\n' e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036 "$build/libc.prx" | sha256sum -c -
cc() { PS5_PAYLOAD_SDK="$sdk" sh "$support/tooling/prospero-clang18" "$@"; }
incs=(-I "$plume" -I "$plume/contrib/volk" -I "$plume/contrib/Vulkan-Headers/include" -I "$plume/contrib/VulkanMemoryAllocator/include")
for stage in vert frag; do
    glslangValidator -V --target-env vulkan1.2 --vn "plume_probe_$stage" \
        "$root/src/plume_probe.$stage" -o "$build/gen/plume_probe_$stage.h"
done
cc -std=c++17 -O2 -DNDEBUG -fexceptions -fcxx-exceptions -fvisibility=hidden -ffunction-sections -fdata-sections \
    "${incs[@]}" -c "$plume/plume_vulkan.cpp" -o "$build/obj/plume_vulkan.o"
cc -std=c++17 -O2 -DNDEBUG -fexceptions -fcxx-exceptions -DDKR_NATIVE_TITLE -Wall -Wextra -Werror -fvisibility=hidden -ffunction-sections -fdata-sections \
    "${incs[@]}" "${extra_incs[@]}" -I "$build/gen" -c "$probe_source" -o "$build/obj/plume_probe.o"
for file in app_crt app_cpp_runtime; do
    cc -std=c++20 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
        -c "$native/$file.cpp" -o "$build/obj/$file.o"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_timegm.c" -o "$build/obj/ps5_timegm.o"
# Keep emulated TLS storage alive while C++ pthread-key destructors run.
# The SDK's LLVM implementation already supports delayed cleanup for Bionic;
# the same ordering is required with the PS5 platform's cxa pthread fallback.
python3 "$root/tools/prepare_emutls.py" "$work/PS5_PayloadSDK/libc/emutls.c" "$build/gen/emutls.c"
cc -std=c11 -O2 -fvisibility=hidden -c "$build/gen/emutls.c" -o "$build/obj/emutls.o"
for library in libSceAgc libSceAgcDriver; do
    if [[ $library == libSceAgc ]]; then file=agc_canary_link_stub.c; else file=agc_driver_canary_link_stub.c; fi
    cc -O2 -fPIC -c "$support/vendor/ps5/sdk/stubs/$file" -o "$build/stubs/$library.o"
    "$sdk/bin/prospero-lld" --shared -soname "$library.prx" -o "$build/stubs/$library.so" "$build/stubs/$library.o"
done
source "$support/tools/radv-link.sh"
radv_link_recipe "$support" "$sdk" "$archive"
"$sdk/bin/prospero-lld" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" \
    --version-script "$native/app-symbols.map" --exclude-libs=ALL -e _start -o "$build/llvm-pie.elf" \
    "$build/obj/app_crt.o" "$build/obj/app_cpp_runtime.o" "$build/obj/plume_probe.o" "$build/obj/plume_vulkan.o" "$build/obj/ps5_timegm.o" "$build/obj/emutls.o" \
    "$build/stubs/libSceAgc.so" "$build/stubs/libSceAgcDriver.so" "${extra_inputs[@]}" "${radv_link_inputs[@]}" \
    --as-needed "$sdk"/target/lib/*.so
"$build/ps5-native-tool" link --in "$build/llvm-pie.elf" --out "$build/eboot.elf" \
    --stub-dir "$sdk/target/lib" --stub "$build/stubs/libSceAgc.so" --stub "$build/stubs/libSceAgcDriver.so" \
    --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf
app="$root/native/PPSA94677"
mkdir -p "$app/sce_sys" "$app/sce_module"
"$build/ps5-native-tool" self --sign --in "$build/eboot.elf" --out "$app/eboot.bin" --magic 0x1D3D154F
cp "$build/libc.prx" "$app/sce_module/libc.prx"
cp "$root/homebrew/DKR-R-Probe/sce_sys/icon0.png" "$app/sce_sys/icon0.png"
chmod 0755 "$app/eboot.bin" "$app/sce_module/libc.prx"
python3 - "$support/sce_sys/param-radv.json" "$app/sce_sys/param.json" "$build_version" <<'PY'
import json,pathlib,sys
p=json.loads(pathlib.Path(sys.argv[1]).read_text())
version=int(sys.argv[3])
p.update(titleId='PPSA94677',conceptId='94677',contentId='UP9000-PPSA94677_00-DKRRGPUPROBE0001',contentVersion=f'01.000.{version:03d}')
p['localizedParameters']['en-US']['titleName']=f'DKR-R GPU Probe v{version}'
pathlib.Path(sys.argv[2]).write_text(json.dumps(p,indent=2)+'\n')
PY
"$build/ps5-native-tool" self --inspect --file "$app/eboot.bin"
"$build/ps5-native-tool" self --inspect --file "$app/sce_module/libc.prx"
echo "Built native FSELF title: $app (diagnostic only; gameplay untested)"
