# DKR-R PS5

An unofficial native PS5 port of [Diddy Kong Racing Recompiled](https://github.com/ThatGuyMcd/DKR-R), based on DKR-R **1.0.6**.

**[Download the source beta](https://github.com/drwellness666/DKR-R-PS5/releases/tag/ps5-beta.2)** · [Deutsch](README.de.md) · [Known issues](docs/KNOWN_ISSUES.md) · [Roadmap](docs/ROADMAP.md)

## What works

- US **1.0 and 1.1 / Rev A**, individually or together; switch under **Touchpad → Play**.
- Original English, French and German game text; shared settings and texture-pack selection, separate progress per revision.
- DualSense controls, vibration and optional held-balloon light colors that return to normal system lighting afterward.
- Full-size menus, PS5 icon/background, optional **4K / 120 Hz and VRR**. Tested on PS5 **13.60**, Relapse / WebKit Autoloader, kstuff-lite and ShadowMountPlus, with an LG C1.

This is a **beta**. Brief water flicker in Tracks previews and occasional microstutters remain reported. See [known issues](docs/KNOWN_ISSUES.md). 120-Hz output does not guarantee 120 rendered FPS in every scene.

## Get started

You need a **Linux x86-64 PC** (or Linux in WSL2), **Python 3.12+**, your own clean **US 1.0 and/or US 1.1 ROM dump**, and a PS5 already able to launch native homebrew with an ELF receiver and ShadowMountPlus. PAL ROMs are unsupported.

1. Download and extract **DKR-R-PS5-source.zip** from the release.
2. Open a terminal in the extracted folder and run:

   ```bash
   python3 setup.py
   ```

3. In the local assistant: **choose your ROM(s) → check the PC → build → enter your PS5's LAN IP → install**. Missing PC prerequisites are listed before building. Then scan `/data/homebrew` in ShadowMountPlus and launch DKR-R from its PS5 icon.

The first build downloads public sources and takes time. All compilation runs on your PC. If you already have a different DKR-R installation, follow the [update guide](docs/UPDATES.md). After a console reboot, use **Start save helper** in the assistant before loading existing progress.

Prefer commands or need details? [Manual build/install](docs/BUILD.md) · [Controls and texture packs](docs/CONTROLS.md) · [Display settings](docs/DISPLAY.md).

## Bugs and contributions

Report problems [here](https://github.com/drwellness666/DKR-R-PS5/issues), with your ROM revision, firmware/loader, port version and steps to reproduce. Keep ROMs, game builds, saves and full console logs out of reports. [Developer guide](docs/DEVELOPMENT.md).

## Credits and licences

Thanks to **ThatGuyMcd and DKR-R contributors**, N64Recomp/N64ModernRuntime, RT64, and the public PS5 SDK/Mesa/RADV projects. Our adapters are [GPL-3.0-or-later](LICENSE); dependencies retain their [own licences](THIRD_PARTY.md).

No ROMs, extracted game assets, generated game code or playable binaries are distributed. Build your own copy from a legally usable dump; keep the resulting game and ROM installers private. Nintendo, Rare, Microsoft and Sony do not endorse this project. [Licence details](docs/LICENSES.md).
