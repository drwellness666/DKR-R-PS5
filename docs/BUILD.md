# Build and install from source

[Deutsch](BUILD.de.md) · [Windows / WSL2](WINDOWS.md) · [Installer target](INSTALLER.md)

The current release builds on a PC and installs a **native PS5 application**.
The installed game does not use PC emulation, streaming or an open build assistant.
The current console-side save-service dependency is described in [Saves](SAVES.md).
A single-ELF installer with on-console ROM selection is planned, not yet delivered.

## Easiest current route

Extract the source ZIP and run `python3 setup.py` in its folder. Use the local
assistant to select supported ROM(s), check prerequisites, build and install.
It never uploads your ROM to GitHub. The command route below is equivalent.

## Requirements

Build host: Linux x86-64, Bash and Python 3.12+. Windows uses [WSL2](WINDOWS.md);
native Windows Python is not enough. Install Make, Git, util-linux/flock,
curl/wget, tar/unzip, patch, pkg-config, PCRE2 development files, ImageMagick,
CMake, Clang/LLD/LLVM development tools, glslang and SPIR-V tools/headers.
The assistant lists missing packages. Source tests also need SDL2 development files.
The checked Linux host used Clang/LLD 23.1.1, CMake 3.31.6 and Python 3.14.
Allow many GB for source/compiler products. Builds do not install OS packages.

PS5: an existing working native-homebrew environment, ShadowMountPlus and an
ELF receiver. The tested setup is firmware 13.60, Relapse / WebKit Autoloader
0.6.1, kstuff-lite (user-reported approximately 1.11), ShadowMountPlus
1.7-beta5fix1, without etaHEN. Other combinations need testing.

| Supported clean 12-MiB dump | Canonical SHA-1 |
|---|---|
| US 1.0 / v77 | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` |
| US 1.1 / Rev A / v80 | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` |

Byte order is detected from contents, not extension. Input files are read-only.
A single-version build needs one dump; a dual build needs both. PAL is unsupported.

## Manual build

Run in Bash from the extracted project folder. Replace ROM paths with your own:

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

Use a fresh `DKR_GAME_WORK` for a new attempt; preserve completed private outputs.
`DKR_PS5_JOBS` / `DKR_GPU_JOBS` limit compilation jobs (default four).
`CMAKE_BIN` selects an installed CMake when needed. Downloads and source patches
are checked against pinned hashes. A failed partial build is not a completed one.

For a single-ROM build, replace the ROM-selection/game-build block with:

```bash
unset DKR_ROM_V77 DKR_ROM_V80
export DKR_ROM="/absolute/path/to/your/own-supported-US-dump.z64"
export DKR_PS5_HIGH_REFRESH_BUILD=1 DKR_PS5_LINE_TABLES=1
bash build_game.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
```

Then run the same installer/helper builder commands. Only included revisions
are available in the resulting app.

## Install on the PS5

Start the normal homebrew environment and ELF receiver. Close DKR-R. The title
`PPSA94678` and `/data/homebrew/PPSA94678` must belong to this port; the installer
refuses different existing files. For an existing version, use [Updates](UPDATES.md).

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP "$DKR_GAME_WORK/private/dkrr-install-game-1.0.6-preview.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP "$DKR_GAME_WORK/private/dkrr-install-owned-rom.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-save-metadata.elf --no-wait
```

Require `NATIVE INSTALL PASS.` and `OWN ROM INSTALL PASS.` from their respective
installers. A transfer alone does not prove installation or gameplay. The final
command starts the **console-side** save service for the current console session;
it does not run a PC service. For the current reboot limitation and an autoloader
workaround, read [Saves](SAVES.md).

Scan `/data/homebrew` in ShadowMountPlus and start **DKR-R 1.0.6 PS5 Preview** from
its PS5 icon. Do not use the anonymous ELF receiver to launch `eboot.bin` as the game.
After installation, closing the PC assistant does not stop the native game.
Keep generated game/ROM installers private. [Controls](CONTROLS.md) ·
[Display](DISPLAY.md) · [Known issues](KNOWN_ISSUES.md) · [Licences](LICENSES.md).
