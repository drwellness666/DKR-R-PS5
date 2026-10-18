# Developer build

Normal installation uses the [ROM-free ELF installer](INSTALLER.md).
This page is for developers and users who want to compile their own copy. The
optional **Start-Windows.cmd** / **bash Start-Linux.sh** assistant still creates a
private ROM-containing installer; do not share that file. The first build
downloads public sources and needs several GB of free space. [Windows setup](WINDOWS.md).

## ROMs and console

| Revision | Size | Canonical SHA-1 |
|---|---|---|
| US 1.0 / v77 | 12 MiB | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` |
| US 1.1 / Rev A / v80 | 12 MiB | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` |

Both are required by the normal installer. Byte order is detected from contents;
original files are read-only. PAL is unsupported. The PS5 must already support
native homebrew, an ELF receiver and ShadowMountPlus. Tested sessions used 13.60,
Relapse / WebKit Autoloader 0.6.1 and kstuff-lite; etaHEN was not used.

## Manual commands (advanced)

Use Linux x86-64/WSL2 with Python 3.12+ and the prerequisites reported by
`python3 tools/host_requirements.py`. Run in Bash from a fresh extracted source
folder; replace the two ROM paths:

```bash
export DKR_PS5_WORK="$PWD/work/sdl"
export DKR_SDL_WORK="$DKR_PS5_WORK"
export DKR_GPU_WORK="$PWD/work/gpu"
export DKR_RT64_WORK="$PWD/work/rt64"
export DKR_GAME_WORK="$PWD/work/game"
export DKR_ROM_V77="/absolute/path/to/owned-US-1.0.z64"
export DKR_ROM_V80="/absolute/path/to/owned-US-1.1.z64"
export DKR_PS5_HIGH_REFRESH_BUILD=1 DKR_PS5_LINE_TABLES=1
bash build.sh
bash build_gpu.sh
bash build_rt64.sh
bash build_integrated.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
python3 tools/build_universal_installer.py \
  "$DKR_SDL_WORK/sdk/ps5-payload-sdk" \
  "$DKR_GAME_WORK/private/native/PPSA94678" \
  "$DKR_GAME_WORK/private/CANDIDATE.json" \
  "$DKR_GAME_WORK/private/universal"
```

Close DKR-R and start the normal console ELF receiver before sending:

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP "$DKR_GAME_WORK/private/universal/DKRR-Installer.elf"
```

Supply both owned ROMs in `USB/DKRR`; require `DKRR UNIVERSAL INSTALL PASS.`.
ShadowMountPlus discovers the registered source; launch the installed PS5 icon.
The sibling `DKRR-Update.elf` uses console-side installation records; no previous
PC work is needed. The first public release supports fresh installation, not
upgrades from private development previews. Before a new published release,
increase `installer_sequence` in `PORT_RELEASE.json`; never reuse one sequence
for different program builds. Use a new output directory for each package.

These two universal ELFs exclude ROMs and extracted controller PCM. Keep the private full app, CANDIDATE.json and matching debug ELF locally. Share
only reviewed ROM-free packages with matching source and original notices.
[Developer guide](DEVELOPMENT.md) · [Licences](LICENSES.md).
