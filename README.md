# DKR-R PS5 🏁

![Diddy Kong Racing Recompiled — original project artwork](https://github.com/user-attachments/assets/c913bee2-029c-4e29-9921-0f6abe7a3bf1)

**Diddy Kong Racing Recompiled, native on PlayStation 5.**
An unofficial port of [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R), created by **ThatGuyMcd and its contributors**.

[📦 Downloads](https://github.com/drwellness666/DKR-R-PS5/releases) · [🇩🇪 Deutsch](README.de.md) · [📖 Help](docs/README.md) · [❤️ Credits](CREDITS.md) · [🐛 Report a bug](https://github.com/drwellness666/DKR-R-PS5/issues)

**Version 1.0 is in preparation.** The final installer and update downloads are not available yet.

## 🎮 Made for PS5

- Start from the PS5 home screen, with the original DKR-R icon and cloud background.
- Switch between **US 1.0 and US 1.1 / Rev A**. Settings and texture packs are shared; progress stays separate.
- Use classic controls and controller-friendly menus, including a diagram of your current button mapping.
- Enable optional **DualSense balloon colors, gyro steering, adaptive L2 feedback and controller sounds** under **Controls → PS5**.
- Choose up to **4K**, optional **HDR10**, or experimental **120 Hz / VRR**. [Display settings](docs/DISPLAY.md).
- Install once and play on the console. Future updates can be installed from the game's settings.

DKR-R provides Modern/Accurate presentation, texture packs, mods, online features and English/French/German game text. The settings menu is English. Compatibility notes are in the [known issues](docs/KNOWN_ISSUES.md).

## 📦 Install

You need a PS5 already running **native homebrew**, **ShadowMountPlus**, an **ELF receiver**, and your own clean **US 1.0 and US 1.1 ROM dumps**. PAL ROMs are unsupported.

1. Download **DKRR-Installer.elf** from a matching release.
2. Put both ROM files in **DKRR** at the root of a USB drive. Their names do not matter.
3. Connect the drive, close DKR-R and send the installer to the PS5's ELF receiver using your usual Windows or Linux sender.
4. Wait for the completion notification and ShadowMountPlus to discover the app, then start **DKR-R** from its PS5 icon.

**No compilation is needed.** After installation, playing and in-game updates need no PC, USB drive or running ELF receiver. Your usual homebrew setup is still required after a console restart.

[Installation help](docs/INSTALLER.md) · [Updating](docs/UPDATES.md) · [Controls and languages](docs/CONTROLS.md)

## 🎨 Optional texture pack

**DKR REMASTERED by sr.gu** is a recommended separate download. Find it through the [official DKR-R Discord](https://discord.com/invite/AMWfXdBjNP), import it under **Textures**, and choose **Modern** presentation. Version 0.2.7 was used on both US revisions. The pack is not included here. [Pack help](docs/CONTROLS.md#texture-packs-and-mods).

## ⚠️ Known issues

- Water can flash briefly when entering a **Tracks preview**.
- Occasional **microstutters** remain, especially with demanding graphics settings or texture packs.
- HDR, the latest installer/update and newer controller features still need their final PS5 checks. Mods, online play and other homebrew setups have limited coverage.

[Compatibility and feature status](docs/KNOWN_ISSUES.md) · [Planned improvements](docs/ROADMAP.md)

## ❤️ Credits and licences

This port depends on the work of **[ThatGuyMcd / DKR-R](https://github.com/ThatGuyMcd/DKR-R)**, **[DavidSM64's decompilation](https://github.com/DavidSM64/Diddy-Kong-Racing)**, **[N64Recomp](https://github.com/N64Recomp)**, **[RT64](https://github.com/rt64/rt64)** and the **public PS5 homebrew community**. The original icon is by **[POOTERMAN](https://www.deviantart.com/pooterman)**.

**[Full credits and project links →](CREDITS.md)** · [Original licences and notices](docs/LICENSES.md)

**AI-assisted / vibe-coded:** this PS5 port was developed with AI assistance. This describes the PS5 integration, not the authorship of the original DKR-R or its dependencies.

The PS5 integration is [GPL-3.0-or-later](LICENSE); other components retain their own terms. Supply your own lawfully usable ROMs. No ROMs, extracted controller audio or texture packs are included in shared downloads. This fan project is not affiliated with or endorsed by Nintendo, Rare, Microsoft or Sony.

Want to compile or contribute? [Developer guide](docs/DEVELOPMENT.md).
