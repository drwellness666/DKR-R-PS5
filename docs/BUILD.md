# DKR-R 1.0.6 — experimental native PS5 source beta

[Deutsche Anleitung](../README.de.md) · [Tested status](../docs/VALIDATION.md) · [Licences](../docs/LICENSES.md)

An unofficial native PS5 port of [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R/releases/tag/Version1.0.6),
using N64Recomp, N64ModernRuntime, RT64 and the public PS5 Mesa/RADV toolchain.
Both supported US revisions can be built into one app. You supply your own legally
usable ROM dump(s) and build locally. The integration does not install a jailbreak.

**This is a source release.** It includes platform adapters, patches, pinned public
source archives, tests and notices. It does not include ROMs, generated game CPU
code, native game binaries/installers, saves, Paks, Sony modules or texture packs.
No build command uploads your inputs or automatically sends a payload to a console.
ROM-free publication does not grant rights to the original game or its assets.

## Current hardware status

The current combined build was installed and started on 15–16 October 2026. The owner
confirmed normal picture/audio, full-size settings at 4K, classic controls, vibration,
save/reload on both revisions and shared settings across normal restarts. US 1.1 loads the
existing remastered archive from the common configuration library. The previous
combined build also had gameplay acceptance for both US revisions, icon, home-menu
cloud background, matched launch art and normal close/restart.

The latest run confirms **4K/119.88-Hz output with VRR Automatic**, with the
unsupported-games override off: PS5 AV output reports VRR48–120 and ALLM, and
the owner confirms normal picture/audio and active VRR on the LG C1. Optional
balloon colors, return to normal system lighting and the aligned version cards
are also owner-confirmed. **120-Hz output remains experimental** across other
hardware and settings; it is not a guarantee of120 rendered FPS in every scene. See
[WORK_STATUS.md](../docs/VALIDATION.md) for this session's remaining acceptance items and
known issues. A brief water flash in the Tracks preview and occasional stutters
remain reported; there is no claim of completely stutter-free operation.

Tested setup: PS5 firmware 13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite
(user reported approximately 1.11), ShadowMountPlus (last captured version
1.7-beta5fix1), no etaHEN. Other firmware/loaders require their own testing.

## Supported ROMs and languages

| Input | Canonical SHA-1 |
|---|---|
| US 1.0 / v77 | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` |
| US 1.1 / Rev A / v80 | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` |

Both clean images are 12 MiB. Byte order is detected from content, independently
of `.z64`, `.n64` or `.v64` suffixes. Unsupported/modified dumps are refused; the
original input is only read. Both versions provide the original English, French
and German language choices through the original game's Options. PAL engines,
PAL timing and a translated Recompiled settings interface are outside this beta.

## Build on Linux x86-64

Use Bash. The tested host has Clang/LLD 23.1.1, CMake 3.31.6 and Python 3.14;
Python 3.12+ is required for safe source extraction. Also install GNU Make, Git,
util-linux (`flock`), curl/wget, tar/unzip, patch, pkg-config, PCRE2 development
files, glslangValidator, ImageMagick, LLVM development files and SPIR-V tools/headers.
Host tests use SDL2 development files. Builds create their own Python environments
and do not install system packages. The first build downloads substantial public
sources and needs many gigabytes of disk space; do not build on the console.

From the extracted source release or a fresh repository checkout:

```bash
export DKR_PS5_WORK="$PWD/work/lowmem"
bash build.sh
export DKR_SDL_WORK="$DKR_PS5_WORK"
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
export DKR_RT64_WORK="$PWD/work/rt64"
bash build_rt64.sh
export DKR_GAME_WORK="$PWD/work/game"
unset DKR_ROM
export DKR_ROM_V77="/absolute/path/to/your/own-US-1.0-dump.z64"
export DKR_ROM_V80="/absolute/path/to/your/own-US-1.1-dump.z64"
bash build_integrated.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
python3 tools/build_native_game_installer.py   "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
python3 tools/build_owned_rom_installer.py   "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
bash tools/build_save_metadata.sh "$DKR_SDL_WORK/sdk/ps5-payload-sdk"
```

Replace both ROM paths. `DKR_PS5_JOBS` / `DKR_GPU_JOBS` limit parallel builds
(default four); `CMAKE_BIN` can choose an installed CMake. Downloads are pinned by
revision and SHA-256. Decompilation, exact rebuilt-ROM verification and the upstream
patch pipeline execute locally. Generated CPU source is not edited manually.

For only one ROM, replace the ROM-selection/game-build part with:

```bash
unset DKR_ROM_V77 DKR_ROM_V80
export DKR_ROM="/absolute/path/to/your/own-supported-US-dump.z64"
export DKR_PS5_HIGH_REFRESH_BUILD=1 DKR_PS5_LINE_TABLES=1
bash build_game.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
```

Run the same installer-builder commands afterwards. A single-ROM app exposes only
its included revision; a complete dual build needs both dumps locally.

## Install and start

Start your already configured jailbreak, ShadowMountPlus and ELF receiver. Close
DKR-R before installation. Title `PPSA94678` and `/data/homebrew/PPSA94678` must not
belong to another app. The installer refuses different existing files. Existing
DKR-R installations use the [incremental update procedure](../docs/UPDATES.md).

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP   "$DKR_GAME_WORK/private/dkrr-install-game-1.0.6-preview.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP   "$DKR_GAME_WORK/private/dkrr-install-owned-rom.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-save-metadata.elf --no-wait
```

These are **your own private outputs**, not GitHub downloads to redistribute.
The ROM installer places only your selected canonical dumps under `/data/dkrr/roms`.
ShadowMountPlus must scan `/data/homebrew` and register the native title. Start
**DKR-R 1.0.6 PS5 Preview** through its PS5 home icon or your configured native
launcher. Do not send `eboot.bin` to the anonymous ELF receiver as a game launcher.

Start the small save-metadata helper again after every console reboot. It reads
only backing-file metadata for the port's own save paths on loopback port 9038,
so strict alias/ownership checks work with the native nullfs view. It never reads
save contents. A completed transfer alone is not proof that the game started.

## Controls, versions and storage

Touchpad opens/closes settings; L1/R1 switch pages, D-pad/left stick select,
Cross confirms and Circle backs out. Options is the original Start button.
Classic controls use Cross for gas, Square for brake, L2 for items/horn and R1
for drift. Gas tapping keeps the original game's digital face-button behavior.
**Controls → Button mapping** still supports custom bindings and restoring Classic.
The former optional trigger-driving preset has been removed. On PS5, an exact
match to that former preset migrates back to Classic; customized mappings stay.
Trigger resistance, fine DualSense haptics, native gyro/speaker output and HDR are
not implemented features of this beta. Standard vibration uses SDL/ScePad.

Optional **Controls → Device → Shared settings → DualSense light bar → Match held balloon color**
uses dim item colors during local play. It starts off, shares its preference
between US revisions and clears the app effect when disabled. Blue is boost,
red missiles, green traps, yellow shield and violet represents the multicolored
magnet balloon. Held-item colors have been confirmed on console. Pause/no item
now requests the native system light-bar reset, preserving the user's normal
off/dim system behavior; held-item colors and system reset are console-confirmed.
Gyro and adaptive triggers are deferred.

**Play → Game version** selects US 1.0 or US 1.1 for the next start. Close/restart
the title normally after selecting. Both versions share controls, graphics,
sound, UI/HUD, mod settings and texture choices in the existing US 1.0 config
root `/data/dkrr/profile-us-v77`. Old US 1.1 configuration files are retained.
**Progress and Controller Paks stay separate per revision** under their original
profile roots. No save-state sharing or EEPROM conversion is promised.

Place your independently obtained, compatible native RT64 texture ZIPs under
`/data/dkrr/profile-us-v77/texture-packs/` and enable them under Textures. Existing
US 1.1 library entries are discovered read-only; no bulk copy is required.
Rice packs can first be converted with the upstream desktop importer; see the
[upstream texture documentation](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/TEXTURE_PACKS.md).
No third-party pack is included or licensed by this release.

HDMI output size and internal rendering resolution are separate settings. 4K
output does not require rendering every game frame at native 4K. Start at 60 Hz
and a modest internal resolution for performance. VRR/ALLM depend on the PS5
system and TV settings. Keep VRR Automatic and Apply to unsupported games Off;
the app declares VRR support. 120-Hz output remains experimental; the displayed
rate is the nominal mode, not an HDMI cadence measurement. Native HDR is absent.

## Updates, diagnostics and licences

[UPDATE_GUIDE.md](../docs/UPDATES.md) explains source updates, local rebuilds and
exact-predecessor updates/rollback without reinstalling ROMs or profiles. There
is no in-game automatic updater or PC GUI installer in this beta.
[DIAGNOSTICS.md](../docs/DIAGNOSTICS.md) describes optional private Klog capture, the own
journal and exact-build symbolization. Do not upload full console logs, ROMs,
private installers, saves or texture packs in bug reports.

Integration adapters are GPL-3.0-or-later; upstream code retains its component
licences and notices. DKR-R runtime licensing is documented upstream; the PS5
interface uses Selawik under SIL OFL 1.1. Artwork is fetched as pinned build input,
not bundled in this source release. Read [LICENSE_REVIEW.md](../docs/LICENSES.md)
and [THIRD_PARTY.md](../THIRD_PARTY.md). This project is not endorsed by Nintendo,
Rare, Microsoft or Sony and does not grant rights to their games, assets or marks.
