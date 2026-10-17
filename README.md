# DKR-R PS5 🏁

![Diddy Kong Racing Recompiled — upstream project artwork](https://github.com/user-attachments/assets/c913bee2-029c-4e29-9921-0f6abe7a3bf1)

**Diddy Kong Racing Recompiled, running natively on PlayStation 5.**
An unofficial PS5 port of [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R).

[📦 Releases](https://github.com/drwellness666/DKR-R-PS5/releases) · [🇩🇪 Deutsch](README.de.md) · [📖 Help](docs/README.md) · [🐛 Report a problem](https://github.com/drwellness666/DKR-R-PS5/issues)

**Version 1.0 is in preparation.** This branch contains all current changes.
The 1.0 installer and update will be released after final verification.
[Progress and test status](docs/VALIDATION.md).

## 🎮 What this port adds

| PS5 addition | What it does |
|---|---|
| Native PS5 app | Start from a PS5 home-screen icon, with matching background and launch artwork. |
| Both US revisions | Choose **US 1.0 or US 1.1 / Rev A** under **Touchpad → Play** for the next launch. |
| Shared setup | Keep controls, picture, sound and texture selections when switching revision. Progress and Controller Paks stay separate. |
| Controller-friendly menus | Original DKR-R typography, full-size interface at 4K, clear version cards, a live controller diagram and compact button mapping. Classic Cross acceleration preserves gas tapping. |
| DualSense extras | Optional held-balloon light colors; gyro steering, L2 item feedback and original item, pickup, coin, horn and boost-pad sounds. Together under **Controls → PS5**. |
| TV output | Optional 1080p, 1440p and **4K**, optional **HDR10**, plus experimental **120 Hz / VRR**. Match display follows the active output; actual FPS depend on the scene and settings. |
| Installation and updates | One ROM-free installer ELF. Later updates can be downloaded in the game or sent as one update ELF. ROM import and controller-sound preparation happen on the PS5. |
| Built-in storage and diagnostics | Direct native save handling, local error journals and a frame-time capture tool. No separate save helper is part of this version. |

Modern/Accurate presentation, HD texture packs, mods, online features and
English/French/German game text come from **upstream DKR-R**. They are retained;
not every pack, mod or online mode has been tested on PS5. The settings UI is English.

## 📦 Install

You need a PS5 already set up for **native homebrew**, an **ELF receiver**,
**ShadowMountPlus**, and both of your own clean **US 1.0 and US 1.1 ROM dumps**.
PAL ROMs are unsupported. Test sessions used firmware 13.60, Relapse / WebKit
Autoloader 0.6.1, kstuff-lite and ShadowMountPlus with an LG C1.

1. Get **DKRR-Installer.elf** from the matching release.
2. Put both ROMs in a folder named **DKRR** at the root of a USB drive. File names do not matter.
3. Connect the drive, close DKR-R and send the installer to your PS5's ELF receiver. Wait for its completion notification.
4. Once ShadowMountPlus discovers the app, start **DKR-R** from its PS5 icon.

After installation, no PC, USB drive or ELF receiver is needed to play or use in-game updates. Your normal
homebrew setup is still required after a console restart. To update, use **Touchpad → About DKR-R → PS5 updates**, or close the game and
send **DKRR-Update.elf** once. Settings, progress and texture packs stay on the
console. No compilation is needed for this ELF route.
[Installation help](docs/INSTALLER.md) · [Updates](docs/UPDATES.md).

## 🎨 Optional texture pack

**DKR REMASTERED by sr.gu** is a recommended optional pack; **v0.2.7** was
confirmed visible on both US revisions in our PS5 sessions. Get it through the
[official DKR-R Discord](https://discord.com/invite/AMWfXdBjNP), which upstream
links for its community texture pack. Import it under **Textures** and use
**Modern** presentation. It is a separate download under its author's terms.
[Pack help](docs/CONTROLS.md#texture-packs-and-mods).

## ⚠️ Known issues

- Brief water flicker when entering a **Tracks preview**; Adventure water was reported stable.
- Occasional **microstutters**, especially with heavier graphics settings or texture packs.
- Upgrading an older registered beta to the new HDR-capable title is still being prepared. Keep your existing installation and saves.
- HDR10 and the newest installation, update and controller features still need their final PS5 checks. See [test status](docs/VALIDATION.md).

[Known issues and compatibility](docs/KNOWN_ISSUES.md).

## 🛠️ Build, credits and licences

Prefer to build your own copy? [Linux / Windows with WSL2](docs/BUILD.md).
[What belongs in this repository](docs/FILES.md).

Thanks to **ThatGuyMcd and the DKR-R contributors**, N64Recomp/N64ModernRuntime,
RT64, the public PS5 SDK and PS5 Vulkan/Mesa/RADV projects. Header artwork is
linked from DKR-R; its application icon is credited upstream to **POOTERMAN**.

The port's adapters are [GPL-3.0-or-later](LICENSE); dependencies retain their
[own licences](THIRD_PARTY.md). ROMs, extracted controller audio, saves and
third-party texture packs are not included in shared downloads. Nintendo, Rare,
Microsoft and Sony do not endorse this project. [Licence details](docs/LICENSES.md).
