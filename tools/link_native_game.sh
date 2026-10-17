#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Link a private ROM-derived game build. Never deploy or include its ROMs.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: link_native_game.sh ABSOLUTE_GPU_WORK ABSOLUTE_GAME_WORK' >&2; exit 2; }
gpu=$1
game=$2
[[ $gpu == /* && $game == /* ]] || exit 2
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
high_refresh=${DKR_PS5_HIGH_REFRESH_BUILD:-0}
[[ $high_refresh == 0 || $high_refresh == 1 ]] || { echo 'DKR_PS5_HIGH_REFRESH_BUILD must be 0 or 1' >&2; exit 2; }
sdl=${DKR_SDL_WORK:-$root/work/lowmem}
[[ $sdl == /* ]] || { echo 'DKR_SDL_WORK must be absolute' >&2; exit 2; }
support="$gpu/PS5_Vulkan"
# The public RADV recipe regenerates a shared map via a fixed .tmp name.
# Serialize game links sharing this GPU workspace, even across revisions.
mkdir -p "$support/build"
exec 9>"$support/build/.dkrr-native-link.lock"
flock 9
sdk="$support/.deps/native/ps5-payload-sdk"
native="$support/tooling/native"
build="$game/native-link"
target="$game/private/native/PPSA94678"
mkdir -p "$build/obj" "$build/gen" "$build/stubs" "$target/sce_sys" "$target/sce_module"
cc() { PS5_PAYLOAD_SDK="$sdk" sh "$support/tooling/prospero-clang18" -gline-tables-only "-ffile-prefix-map=$root=/src/dkrr-ps5" "$@"; }
build_id=$(python3 "$root/tools/native_build_identity.py" "$root" "$game" "$sdl" "$support" "$build/native-inputs.json" --high-refresh "$high_refresh")
profile="$game/game-ps5/dkr-ps5-profile.json"
[[ -f $profile ]] || { echo 'Missing selected-revision CMake profile' >&2; exit 2; }
mapfile -t revisions < <(python3 -c 'import json,sys;p=json.load(open(sys.argv[1]));print(p["v77"]);print(p["v80"])' "$profile")
release_sequence=$(python3 -c 'import json,sys;v=json.load(open(sys.argv[1]))["installer_sequence"];assert type(v) is int and 1<=v<=2147483647;print(v)' "$root/PORT_RELEASE.json")
cc -DDKR_PAYLOAD_V77_ENABLED="${revisions[0]}" -DDKR_PAYLOAD_V80_ENABLED="${revisions[1]}" -std=c++20 -O2 -fexceptions -fcxx-exceptions -D_LIBCPP_ENABLE_EXPERIMENTAL -Wall -Wextra -Werror \
    -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -c "$root/src/ps5_game_main.cpp" -o "$build/obj/game_main.o"
cc -DDKR_PAYLOAD_V77_ENABLED="${revisions[0]}" -DDKR_PAYLOAD_V80_ENABLED="${revisions[1]}" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -c "$root/src/ps5_revision_selection.cpp" -o "$build/obj/revision_selection.o"
cc -DDKR_PS5_HIGH_REFRESH_BUILD="$high_refresh" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -c "$root/src/ps5_display_options.cpp" -o "$build/obj/display_options.o"
cc -std=c++20 -O2 -Wall -Wextra -Werror -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -c "$root/src/ps5_display_videoout.cpp" -o "$build/obj/display_videoout.o"
for file in app_crt app_cpp_runtime; do
    input="$native/$file.cpp"
    [[ $file != app_crt ]] || input="$root/src/ps5_game_crt.cpp"
    cc -DDKR_BUILD_ID=\""$build_id"\" -I "$root/src" -std=c++20 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
        -c "$input" -o "$build/obj/$file.o"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -c "$root/src/ps5_diagnostics.c" -o "$build/obj/diagnostics.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_timegm.c" -o "$build/obj/timegm.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_posix_madvise.c" -o "$build/obj/madvise.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_realpath.c" -o "$build/obj/realpath.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_lstat.c" -o "$build/obj/lstat.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_getcwd.c" -o "$build/obj/getcwd.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_pathconf.c" -o "$build/obj/pathconf.o"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -c "$root/src/ps5_rdram.c" -o "$build/obj/rdram.o"
cc -std=c++20 -O2 -Wall -Wextra -Werror -fexceptions -fcxx-exceptions \
    -I "$root/src" -c "$root/src/ps5_controller_speaker.cpp" -o "$build/obj/controller_speaker.o"
cc -DDKR_PAYLOAD_V77_ENABLED="${revisions[0]}" -DDKR_PAYLOAD_V80_ENABLED="${revisions[1]}" -std=c++20 -O2 -Wall -Wextra -Werror -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I "$root/src" -I "$game/source/N64Recomp/include" -c "$root/src/ps5_game_audio_hooks.cpp" -o "$build/obj/game_audio_hooks.o"
cc -std=c++20 -O2 -Wall -Wextra -Werror -fexceptions -fcxx-exceptions -fvisibility=hidden \
    -DDKRR_RELEASE_SEQUENCE="$release_sequence" -I "$root/src" \
    -c "$root/src/ps5_internet_update.cpp" -o "$build/obj/internet_update.o"
cc -std=c++20 -O2 -Wall -Wextra -Werror -fvisibility=hidden \
    -DMBEDTLS_CONFIG_FILE=\"legacy_mod_hash_config.h\" -I "$root/src" \
    -I "$game/source/DKR-R/extern/monocypher/src" -I "$game/source/DKR-R/extern/monocypher/src/optional" \
    -I "$game/source/DKR-R/extern/mbedtls/include" -I "$game/source/DKR-R/runtime-recomp/src/game/mods" \
    -c "$root/src/ps5_update_policy.cpp" -o "$build/obj/update_policy.o"
cc -std=c11 -O2 -fvisibility=hidden \
    -I "$game/source/DKR-R/extern/monocypher/src" \
    -c "$game/source/DKR-R/extern/monocypher/src/optional/monocypher-ed25519.c" -o "$build/obj/update_ed25519.o"
python3 "$root/tools/prepare_emutls.py" "$gpu/PS5_PayloadSDK/libc/emutls.c" "$build/gen/emutls.c"
cc -std=c11 -O2 -fvisibility=hidden -c "$build/gen/emutls.c" -o "$build/obj/emutls.o"
for lib in libSceAgc libSceAgcDriver; do
    if [[ $lib == libSceAgc ]]; then file=agc_canary_link_stub.c; else file=agc_driver_canary_link_stub.c; fi
    cc -O2 -fPIC -c "$support/vendor/ps5/sdk/stubs/$file" -o "$build/stubs/$lib.o"
    "$sdk/bin/prospero-lld" --shared -soname "$lib.prx" -o "$build/stubs/$lib.so" "$build/stubs/$lib.o"
done
mapfile -d '' libs < <(find "$game/game-ps5" -type f -name '*.a' -print0 | sort -z)
filtered_libs=()
for lib in "${libs[@]}"; do
    [[ ${revisions[0]} != 0 || $lib != */libDKRRecompiledFuncsV77.a ]] || continue
    [[ ${revisions[1]} != 0 || $lib != */libDKRRecompiledFuncsV80.a ]] || continue
    filtered_libs+=("$lib")
done
libs=("${filtered_libs[@]}")
[[ -f $game/game-ps5/libDKR-R.a && -f $game/game-ps5/rt64/rt64.a ]] || { echo 'Game and renderer libraries missing' >&2; exit 2; }
source "$support/tools/radv-link.sh"
radv_link_recipe "$support" "$sdk" "$gpu/radv-build/src/amd/vulkan/libvulkan_radeon.a"
radv_link_flags+=(--wrap=mmap --wrap=mprotect --wrap=munmap --wrap=_ZN11ultramodern14error_handling11message_boxEPKc)
radv_link_flags+=(--wrap=sceVideoOutOpen --wrap=sceVideoOutIsOutputSupported)
for prefix in '' dkr_v80_; do
    [[ $prefix != '' || ${revisions[0]} != 0 ]] || continue
    [[ $prefix != dkr_v80_ || ${revisions[1]} != 0 ]] || continue
    for symbol in obj_loop_weaponballoon obj_loop_silvercoin obj_loop_goldenballoon racer_play_sound sound_play audspat_play_sound_at_position music_jingle_play sound_play_spatial; do
        radv_link_flags+=("--wrap=${prefix}${symbol}")
    done
done
printf '{ local: _Zn*; _Zd*; getargv; realpath; lstat; getcwd; pathconf; __wrap_mmap; __wrap_mprotect; __wrap_munmap; };\n' > "$build/gen/game-symbols.map"
link_game() {
"$sdk/bin/prospero-lld" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" \
    --version-script "$build/gen/game-symbols.map" --exclude-libs=ALL -e _start -o "$build/llvm-pie.elf" \
    "$build/obj/app_crt.o" "$build/obj/app_cpp_runtime.o" "$build/obj/game_main.o" "$build/obj/revision_selection.o" "$build/obj/display_options.o" "$build/obj/display_videoout.o" "$build/obj/diagnostics.o" "$build/obj/timegm.o" "$build/obj/emutls.o" "$build/obj/madvise.o" "$build/obj/realpath.o" "$build/obj/lstat.o" "$build/obj/getcwd.o" "$build/obj/pathconf.o" "$build/obj/rdram.o" "$build/obj/import_check.o" \
    "$build/obj/controller_speaker.o" "$build/obj/game_audio_hooks.o" "$build/obj/internet_update.o" "$build/obj/update_policy.o" "$build/obj/update_ed25519.o" "$build/stubs/libSceAgc.so" "$build/stubs/libSceAgcDriver.so" \
    --start-group "${libs[@]}" "$sdl/build/sdl/libSDL2.a" "$sdl/build/samplerate/src/libsamplerate.a" --end-group \
    "${radv_link_inputs[@]}" --no-as-needed "$sdk/target/lib/libSceHttp.so" "$sdk/target/lib/libSceSsl.so" "$sdk/target/lib/libSceNet.so" --as-needed "$sdk"/target/lib/*.so
}
# First pass obtains imports without introducing new library dependencies.
printf 'unsigned dkrr_report_missing_imports(void (*emit)(const char *)){(void)emit;return 0;}\n' > "$build/gen/import_check.c"
cc -std=c11 -O2 -fvisibility=hidden -c "$build/gen/import_check.c" -o "$build/obj/import_check.o"
link_game
python3 "$root/tools/generate_native_import_check.py" "$build/llvm-pie.elf" "$build/gen/import_check.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden -c "$build/gen/import_check.c" -o "$build/obj/import_check.o"
link_game
tool="$gpu/native-title/ps5-native-tool"
[[ -x $tool ]] || { echo 'Build the native renderer first to obtain the public converter' >&2; exit 2; }
"$tool" link --in "$build/llvm-pie.elf" --out "$build/eboot.elf" \
    --stub-dir "$sdk/target/lib" --stub "$build/stubs/libSceAgc.so" --stub "$build/stubs/libSceAgcDriver.so" \
    --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf
"$tool" self --sign --in "$build/eboot.elf" --out "$target/eboot.bin" --magic 0x1D3D154F
cp "$gpu/native-title/libc.prx" "$target/sce_module/libc.prx"
bash "$root/tools/prepare_title_art.sh" "$gpu" "$game"
for image in icon0.png pic0.dds pic1.dds; do
    cp "$game/private/title-art-paired/$image" "$target/sce_sys/$image"
done
chmod 0755 "$target/eboot.bin" "$target/sce_module/libc.prx"
[[ -d $game/game-ps5/assets/filters && -f $game/game-ps5/assets/controllers/gamecontrollerdb.txt ]] || {
    echo 'CMake-staged game assets missing' >&2; exit 2;
}
cp -a "$game/game-ps5/assets" "$target/"
python3 "$root/tools/prepare_controller_audio.py" "$game/private/roms" "$target/assets/controller-audio" \
    > "$build/controller-audio-receipt.txt"
mkdir -p "$target/assets/licenses"
cp "$root/licenses/DKR-R-LICENSE.md" "$target/assets/licenses/DKR-R-MIT.md"
cp "$root/licenses/DKR-R-Selawik-OFL.txt" "$target/assets/licenses/Selawik-OFL.txt"
cp "$root/LICENSE" "$target/assets/licenses/GPL-3.0.txt"
cp "$root/THIRD_PARTY.md" "$target/assets/licenses/THIRD_PARTY.md"
cp "$root/licenses/LLVM-libcxx.txt" "$target/assets/licenses/LLVM-libcxx.txt"
cp "$root/licenses/VADPCM-MPL-2.0.txt" "$target/assets/licenses/VADPCM-MPL-2.0.txt"
cp "$root/licenses/Monocypher.md" "$target/assets/licenses/Monocypher.md"
cp "$root/licenses/Mbed-TLS.md" "$target/assets/licenses/Mbed-TLS.md"
cp "$root/licenses/SDL2.txt" "$target/assets/licenses/SDL2.txt"
cp "$root/licenses/libsamplerate.txt" "$target/assets/licenses/libsamplerate.txt"
cp "$root/homebrew/DKR-R-Game/ARTWORK.md" "$target/assets/licenses/Title-Artwork.md"
cp "$root/TITLE_ART_SOURCES.json" "$target/assets/licenses/Title-Artwork-Sources.json"
# A reused early preview may contain the notice for the rejected generic art.
# Preserve that known old notice outside the app; it does not cover upstream art.
python3 - "$target/assets/licenses/Title-Artwork-CC0.txt" "$root/licenses/CC0-1.0.txt" "$build/obsolete-generic-art-CC0.txt" <<'PY'
from pathlib import Path
import sys
old, reference, backup = map(Path, sys.argv[1:])
if old.exists() or old.is_symlink():
    if old.is_symlink() or not old.is_file() or old.read_bytes() != reference.read_bytes():
        raise SystemExit('Unexpected old artwork notice; preserve and inspect the private output')
    if backup.exists():
        if backup.is_symlink() or not backup.is_file() or backup.read_bytes() != old.read_bytes():
            raise SystemExit('Old artwork notice backup collision')
        old.unlink()
    else:
        old.rename(backup)
PY

python3 - "$support/sce_sys/param-radv.json" "$target/sce_sys/param.json" "$game/game-ps5/dkr-release.json" "$high_refresh" <<'PY'
import json,pathlib,sys
p=json.loads(pathlib.Path(sys.argv[1]).read_text())
p.update(titleId='PPSA94678',conceptId='94678',contentId='UP9000-PPSA94678_00-DKRRNATIVE000001',contentVersion='01.000.001')
version=json.loads(pathlib.Path(sys.argv[3]).read_text())['version']
p['localizedParameters']['en-US']['titleName']='DKR-R '+version+' PS5 Preview'
# Explicit experimental build only. The pinned VideoOut WSI reads 0x40;
# its public source describes 0x40000 as the VRR-compatible declaration.
# Runtime activation still requires a saved 120 Hz choice, positive native
# support and the WSI's measured vblank check. No legacy 0x80000 flag.
if sys.argv[4] == '1':
    p['attribute3'] = p.get('attribute3', 0) | 0x40040
pathlib.Path(sys.argv[2]).write_text(json.dumps(p,indent=2)+'\n')
PY
"$tool" self --inspect --file "$target/eboot.bin"
echo "Built private native game candidate: $target (hardware gameplay untested)"
