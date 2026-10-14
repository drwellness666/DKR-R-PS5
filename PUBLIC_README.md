# DKR-R 1.0.6 — experimental native PS5 port

Build an unofficial native PS5 version of
[DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R/releases/tag/Version1.0.6)
with **US 1.0 and US 1.1 in one app**, using your own legally usable dumps
locally. A build with only one supported US dump remains possible.
The PS5 integration uses a native x86-64 recompiled CPU payload, RT64 and a
public Mesa/RADV homebrew toolchain. It is not a packaged emulator.

This repository contains integration source, platform patches, build tools,
tests, source hashes and licence notices. It contains **no ROM, generated game
CPU payload, game executable, save, Controller Pak, texture pack or Sony system
module**. Builds stay under the ignored `work/` directory. No build step uploads
your ROM or automatically deploys anything to a console.

## Private source beta

Start here: [PRIVATE_BETA.md](PRIVATE_BETA.md). This repository initially remains
private. Tester access is granted separately; no private game build is uploaded.

## Current status

The owner confirmed both US revisions in the same integrated app, normal image
and audio, readable save-slot labels, the original icon, the selected-title cloud
background and matched launch artwork. A normal close and second launch passed.
4K buffers ran at 59.94 Hz; requested 120 Hz fell back after the WSI cadence
check. Complete save/Pak reload and the optional driving preset remain pending.

The new offline refinement adds full-size 4K menu scaling, default queue-statistic
log suppression, a bounded high-refresh transition retry and qualified journal
provision/read-back. An exact own-pack copier is prepared for the US 1.1 profile;
no texture bytes are embedded or distributed. The full native build passes,
while physical acceptance of these latest changes is still pending.

Known issues include the brief Tracks-preview water flash, unqualified
Adventure intro behavior and occasional stutters. Use this as an experimental
source preview. [WORK_STATUS.md](WORK_STATUS.md) separates physical observations
from new host-tested refinements. Advanced DualSense effects and native HDR
remain documented backend work; no nonfunctional enable controls are added.

## Supported ROMs

| Local input | Canonical SHA-1 | Build requirement |
|---|---|---|
| US v1.0 / `us.v77` | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` | This single dump is sufficient |
| US Rev A / v1.1 / `us.v80` | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` | This single dump is sufficient |

Both are clean 12 MiB images. The checker accepts supported N64 byte orders
regardless of the filename suffix and creates a private canonical working copy.
PAL, Japanese, modified and unsupported dumps are rejected. The original input
is not changed. The main recipe builds both supported revisions when you have
both legally usable dumps; a single-ROM build is also supported.

## Build on Linux

Read [GAME_BUILD.md](GAME_BUILD.md) for prerequisites, installation and the
pending console test plan. The tested host is Linux x86-64 with Clang/LLD 23.1.1,
CMake 3.31.6 and Python 3.14; source extraction requires Python 3.12 or newer.
Additional host requirements include GNU Make, Git, util-linux (`flock`), curl/wget, unzip/tar,
patch, pkg-config, PCRE2 development files, glslangValidator, ImageMagick (`magick` or `convert`), LLVM development
files and SPIR-V tools/headers. Host adapter tests also use SDL2 development files.
Python build dependencies are installed into dedicated virtual environments.
The build does not install system packages.

From a fresh checkout, in **Bash**:

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
export DKR_ROM_V77="/absolute/path/to/your/own/US-1.0-dump.z64"
export DKR_ROM_V80="/absolute/path/to/your/own/US-1.1-dump.z64"
bash build_game.sh
python3 tools/build_native_game_installer.py \
  "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
python3 tools/build_owned_rom_installer.py \
  "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
bash tools/build_save_metadata.sh "$DKR_SDL_WORK/sdk/ps5-payload-sdk"
```

Replace both ROM paths. For a single revision, set `DKR_ROM` instead and unset
`DKR_ROM_V77`/`DKR_ROM_V80`. `DKR_GPU_JOBS` and `DKR_PS5_JOBS` limit parallel builds
(default four); `CMAKE_BIN` can select an installed CMake. Public source downloads
are pinned by revision and SHA-256. GNU binutils, matching decompilation and the
upstream CPU Patch Pipeline run locally for only the selected ROM revision.
The rebuilt ROM must match its known SHA-1. Generated CPU source is not edited.

The first candidate uses Vulkan, 2× original resolution, original frame rate,
four renderer workers and no MSAA. Enhanced Water/RTX, SDL3 helper processes,
WebRTC Quick Join and the external legacy importer are disabled.

## Install your private build

Your console must already have a suitable native homebrew launch environment and
an ELF receiver. Verify the title/path collision precautions in
[GAME_BUILD.md](GAME_BUILD.md) before installation on another console.

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP \
  "$DKR_GAME_WORK/private/dkrr-install-game-1.0.6-preview.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP \
  "$DKR_GAME_WORK/private/dkrr-install-owned-rom.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP \
  bin/dkrr-save-metadata.elf --no-wait
```

These are **your own local outputs**, not files supplied in a GitHub release.
The second installer contains your ROM and must never be shared. The game
installer contains ROM-derived executable code and must also remain private.
Both refuse different existing destination files. They do not alter jailbreak
configuration or launch another application.

ShadowMount-Plus must register `/data/homebrew/PPSA94678/`. Launch
**DKR-R 1.0.6 PS5 Preview** as a native title from the console menu or your already
configured native launcher. Sending the game's `eboot.bin` directly to an
anonymous ELF loader is not the supported GPU launch method.

The provisional save-metadata helper listens only on the console's loopback
interface and reads only metadata for the port's own save paths. It is needed
on the tested native filesystem view to preserve the unique-link checks.
Start it again after a console reboot. The current helper remains available across
long pauses until it is stopped or the console reboots; peer requests have timeouts.
Without it, affected existing saves/Paks may be refused rather than overwritten.
Full save/load acceptance remains pending.

## Controls and beta testing

Touchpad opens/closes settings; L1/R1 switch pages, D-pad/left stick select,
Cross confirms and Circle backs out of an active control/popup or returns to the
race from an idle page. The new PS5 menu uses one vertical flow for ordinary
settings fields and cards. The local test checks actual ImGui focus order,
disabled items, scrolling and Cross activation. TV acceptance is still pending.

The build prepares the actual pinned DKR-R launcher icon and README header as
a 512px icon plus identical 4K BC7 selection and launch backgrounds. The
launch overlay is released after the first completed game presentation. The originals are
verified build inputs; generated character art and retouch drafts are excluded. [CONSOLE_FEATURES.md](CONSOLE_FEATURES.md) explains the existing
English/French/German directly in the original game Options for both US revisions, virtual Controller Paks and
DualSense rumble. Their newest candidate-specific visual/persistence/physical
acceptance is still pending. PAL engines and a German Recompiled interface are
not included. [RENDER_ISSUES.md](RENDER_ISSUES.md) tracks missing game labels,
Tracks-preview water flicker and stutters with opt-in A/B diagnostics.

Read [BETA_TESTING.md](BETA_TESTING.md) before reporting a result. Please identify
your ROM **revision/hash**, build commit and console/loader versions. Attach
only relevant filtered diagnostics. Do not attach ROMs, generated CPU code,
private installers, saves, full console dumps or third-party texture packs.

## Licences and publication scope

Our integration adapters are GPL-3.0-or-later. Original DKR-R project code is MIT;
N64ModernRuntime and parts of the public platform runtime have GPL-3.0 obligations.
Other components retain their own licences. Original notices are preserved in
source archives and the readable [licenses](licenses/) directory.

The PS5 build now embeds only **Selawik under SIL OFL 1.1** for its interface fonts;
Racing Banana and Jumpman are excluded from its link inputs. They remain part of
upstream desktop code and are not redistributed in this integration export.
ROM-free source publication does not grant rights to the original game or to
third-party artwork. See [LICENSE_REVIEW.md](LICENSE_REVIEW.md),
[THIRD_PARTY.md](THIRD_PARTY.md) and the
[upstream asset policy](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md).
Nintendo, Rare and Sony do not endorse this port.

## Checks and contributions

[CHECKS.json](CHECKS.json) and [VERIFICATION.md](VERIFICATION.md) distinguish
local build evidence from console and user-visible acceptance. The default
GitHub workflow checks the source export and host adapters without a ROM or
matchmaking connection. Optional checks after source preparation are documented
in `VERIFICATION.md`.

[WORK_STATUS.md](WORK_STATUS.md) lists the remaining work and next console
acceptance sequence. [RENDER_ISSUES.md](RENDER_ISSUES.md) documents the opt-in
upstream timing reports and local numerical log summary. The app-bundle update
recipe in [GAME_BUILD.md](GAME_BUILD.md) also handles changed title artwork;
it requires a verified preceding build and never connects to a console itself.

Keep changes in reviewable platform adapters and patches. Preserve authorship
and exact dependency revisions. Never commit private build outputs, even
briefly: removing them from a later commit does not remove them from Git history.
Before exporting a release draft, run the clean-source scan:

```bash
python3 tools/check_public_source.py .
```

Run this on a clean source export, before creating local build artifacts.
For further development, read [PUBLICATION.md](PUBLICATION.md).

## Latest native start fix and revision selector

The native game now releases the PS5 launch overlay once after its first
completed presentation, using the same API as the accepted renderer probes.
The corrective US 1.0 build was tested on the console: the user confirms picture
and normal audio. This avoids a launch poster remaining over a running game.
The owner subsequently confirmed the separate home-menu cloud background and
matched launch image after a fresh ordinary ShadowMount title registration.

In a dual-ROM build, **Touchpad → Play → Game version** offers **US 1.0** and
**US 1.1 – Rev A** for the next title launch. Each keeps its existing separate
profile. A one-ROM build stays on its included revision. The adapter has host
acceptance tests and the corrected dual native build compiles/links; the owner subsequently selected Rev A, closed/restarted the same app and
confirmed gameplay. The optional trigger preset still needs explicit activation
and its own physical test.
Microstutters and the short Tracks-preview water flash remain reported defects.
The normal build does not enable extra timing capture.

## Current offline scope

The candidate keeps both US revisions and the checked three-language original
Options cycle. PAL integration is deferred by the owner's choice. The preceding
integrated candidate was accepted for both revisions, normal picture/audio,
paired artwork and repeated start. The console is now explicitly powered off.
The new full-size 4K UI, quiet driver statistics, bounded high-refresh retry
and own-profile support helpers are offline-qualified and not deployed.

[TITLE_PRESENTATION.md](TITLE_PRESENTATION.md) records the paired image policy
and accepted ordinary fresh registration. Actual Play-page navigation and
absence of the extra sidebar marker geometry pass local tests. Remaining
physical checks are listed in [INTEGRATED_TEST.md](INTEGRATED_TEST.md).

## Performance and compatible updates

The PS5 neutral-audio path now performs exact integer stereo conversion rather
than per-sample floating point/rounding work. It is tested against the actual
upstream mixer for every int16 value. Equalizer changes, volume changes, custom
music and UI tones retain the original mixer; guest AI timing and SDL queue
priming remain unchanged. This is a host-evidenced cost reduction, not a measured
PS5 stutter fix. A guarded `shader_cache=1` opt-in exposes the documented RADV
exclusive disk cache for a cold/warm comparison; it remains off by default.

[UPDATE_GUIDE.md](UPDATE_GUIDE.md) documents verified port-source release checking,
local private rebuilds and incremental updates with an exact rollback. Unchanged
ROMs and saved progress stay in place. The beta has no in-game automatic installer.

[DUALSENSE_CONCEPT.md](DUALSENSE_CONCEPT.md) audits the native controller backend
and proposes optional gyro, light, adaptive-trigger and haptic improvements.
These are future features, not active promises of this beta.

The integrated candidate contains a per-player **R2 / L2 driving** preset,
classic layout/undo, PlayStation button names and vertically navigated binding fields.
[Mapping and local test boundaries](CONSOLE_FEATURES.md#optional-dualsense-driving-layout).
Physical PS5 acceptance is pending; this does not advertise adaptive resistance or
native gyro. The published beta.1 snapshot remains available unchanged.

Optional native output choices reuse **Graphics → Game display**: 1080p / 1440p / 4K and, in the integrated candidate, 60 / 120 Hz. Explicit Apply takes effect next launch; no saved choice defaults to 1080p/60. Existing render-resolution and presentation-rate controls remain independent. VRR/ALLM follow system/TV settings; native HDR and advanced DualSense actuator support are not claimed. See [DISPLAY_FEATURES.md](DISPLAY_FEATURES.md).

The current combined test track is documented in [INTEGRATED_TEST.md](INTEGRATED_TEST.md). It brings both US revisions, languages, controller/menu fixes, supported output choices and diagnostics into one app. [DIAGNOSTICS.md](DIAGNOSTICS.md) explains crash journals, optional bounded frame-time capture and an optional PC kernel-log relay. Raw journals, matching symbol ELFs and native game binaries remain private. Physical acceptance and the reported preview-water flash/microstutters remain open.
