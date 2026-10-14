#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds the Plume/RADV native integration probe. Does not deploy it.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${DKR_GPU_WORK:?Set DKR_GPU_WORK to an absolute dedicated build directory}"
[[ $DKR_GPU_WORK == /* ]] || { echo 'DKR_GPU_WORK must be absolute' >&2; exit 2; }
jobs=${DKR_GPU_JOBS:-4}
work=$DKR_GPU_WORK
mkdir -p "$work"
for tool in python3 clang clang++ llvm-config glslangValidator patch make wget unzip sha256sum; do
    command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 2; }
done
if [[ ! -x $work/host-tools/bin/python ]]; then python3 -m venv "$work/host-tools"; fi
if [[ ! -f $work/host-tools/.dkrr-dependencies-v1 ]]; then
    "$work/host-tools/bin/python" -m pip install meson==1.9.1 ninja==1.13.0 mako==1.3.10 pyyaml==6.0.3 ply==3.11 packaging==25.0
    touch "$work/host-tools/.dkrr-dependencies-v1"
fi
export PATH="$work/clc-bin:$work/host-tools/bin:$PATH"
python3 "$root/tools/fetch_vulkan_sources.py" "$work"
mesa="$work/PS5_Mesa"
support="$work/PS5_Vulkan"
sdk="$support/.deps/native/ps5-payload-sdk"
bash "$work/PS5_PayloadSDK/platform/tools/setup-sdk.sh" "$sdk" b83202be73e930050e00de0f1b4d9ec46c0391bf "$work/downloads"
python3 "$root/tools/apply_plume_patch.py" "$work/Plume"
mkdir -p "$mesa/subprojects/packagecache"
cp "$root/vendor/zlib-1.3.1.tar.gz" "$root/vendor/zlib_1.3.1-1_patch.zip" "$mesa/subprojects/packagecache/"
if [[ ! -f $work/clc-build/build.ninja ]]; then
    CC=clang CXX=clang++ meson setup "$work/clc-build" "$mesa" \
        -Dbuildtype=release -Dmesa-clc=enabled -Dinstall-mesa-clc=true \
        -Dgallium-drivers= -Dvulkan-drivers= -Dplatforms= -Dglx=disabled -Degl=disabled -Dgbm=disabled \
        -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dllvm=enabled -Dshared-llvm=enabled \
        -Dbuild-tests=false -Dvalgrind=disabled -Dlibunwind=disabled -Dzstd=disabled -Dxmlconfig=disabled -Dtools=
fi
ninja -C "$work/clc-build" -j "$jobs" src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2
mkdir -p "$work/clc-bin"
ln -sf "$work/clc-build/src/compiler/clc/mesa_clc" "$work/clc-bin/mesa_clc"
ln -sf "$work/clc-build/src/compiler/spirv/vtn_bindgen2" "$work/clc-bin/vtn_bindgen2"
printf "[constants]\nsdk = '%s'\n" "$sdk" > "$work/radv-cross-constants.ini"
if [[ ! -f $work/radv-build/build.ninja ]]; then
    meson setup "$work/radv-build" "$mesa" \
        --cross-file "$work/radv-cross-constants.ini" --cross-file "$support/tooling/radv/ps5-cross.ini" \
        -Dvulkan-drivers=amd -Dgallium-drivers= -Dplatforms= -Dradv-winsys=ps5 \
        -Dllvm=disabled -Damd-use-llvm=false -Dvideo-codecs= -Dbuildtype=debugoptimized -Db_ndebug=true \
        -Dglx=disabled -Degl=disabled -Dgbm=disabled -Dopengl=false -Dgles1=disabled -Dgles2=disabled \
        -Dvalgrind=disabled -Dlibunwind=disabled -Dzstd=disabled -Dzlib=enabled \
        --force-fallback-for=zlib --wrap-mode=nodownload -Dexpat=disabled -Dxmlconfig=disabled \
        -Dshader-cache=enabled -Dbuild-tests=false -Dvulkan-layers= -Dtools= -Dmesa-clc=system \
        -Dradv-build-id=47e9eb61ee7e1112194557e1e047178562827bc6
fi
ninja -C "$work/radv-build" -j "$jobs" src/amd/vulkan/libvulkan_radeon.a
bash "$root/tools/link_gpu_probe.sh" "$work"
