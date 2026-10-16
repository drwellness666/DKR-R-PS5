# DKR-R PS5 🏁

![Diddy Kong Racing Recompiled — upstream project artwork](https://github.com/user-attachments/assets/c913bee2-029c-4e29-9921-0f6abe7a3bf1)

**Diddy Kong Racing Recompiled, running natively on PlayStation 5.**
An unofficial PS5 port of [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R).

[📦 Releases](https://github.com/drwellness666/DKR-R-PS5/releases) · [🇩🇪 Deutsch](README.de.md) · [📖 Help](docs/README.md) · [🐛 Report a problem](https://github.com/drwellness666/DKR-R-PS5/issues)

**Beta 3 preview.** Gameplay has been tested on PS5. The new USB installer,
native save backend and additional controller effects are included in this
preview; their PS5 testing is still in progress. [Test status](docs/VALIDATION.md).

## 🎮 What this port adds

| PS5 addition | What it does |
|---|---|
| Native PS5 app | Start from a PS5 home-screen icon, with matching background and launch artwork. |
| Both US revisions | Choose **US 1.0 or US 1.1 / Rev A** under **Touchpad → Play** for the next launch. |
| Shared setup | Keep controls, picture, sound and texture selections when switching revision. Progress and Controller Paks stay separate. |
| Controller-friendly menus | Full-size interface at 4K, clear version cards and D-pad navigation. Classic Cross acceleration preserves gas tapping. |
| DualSense extras | Optional held-balloon light colors; experimental gyro steering, L2 item feedback and item sounds from the controller speaker. Together under **Controls → PS5**. |
| TV output | Optional 1080p, 1440p and **4K**, plus experimental **120 Hz / VRR**. Match display follows the active output; actual FPS depend on the scene and settings. |
| Installation and updates | One ROM-free installer ELF, then one update ELF for later versions. ROM import and controller-sound preparation happen on the PS5. |
| Built-in storage and diagnostics | Direct native save handling, local error journals and a frame-time capture tool. No separate save helper is part of this version. |

Modern/Accurate presentation, HD texture packs, mods, online features and
English/French/German game text come from **upstream DKR-R**. They are retained;
not every pack, mod or online mode has been tested on PS5. The settings UI is English.

## 📦 Install

You need a PS5 already set up for **native homebrew**, an **ELF receiver**,
**ShadowMountPlus**, and both of your own clean **US 1.0 and US 1.1 ROM dumps**.
PAL ROMs are unsupported. Test sessions used firmware 13.60, Relapse / WebKit
Autoloader 0.6.1, kstuff-lite and ShadowMountPlus with an LG C1.

1. Get **DKRR-Installer.elf** from the beta release.
2. Put both ROMs in a folder named **DKRR** at the root of a USB drive. File names do not matter.
3. Connect the drive, close DKR-R and send the installer to your PS5's ELF receiver. Wait for its completion notification.
4. Once ShadowMountPlus discovers the app, start **DKR-R** from its PS5 icon.

After installation, the PC and USB drive are not needed to play. Your normal
homebrew setup is still required after a console restart. To update, close the
game and send the new **DKRR-Update.elf** once. Settings, progress and texture
packs stay on the console. No compilation is needed for this ELF route.
[Installation help](docs/INSTALLER.md) · [Updates](docs/UPDATES.md).

## ⚠️ Known issues

- Brief water flicker when entering a **Tracks preview**; Adventure water was reported stable.
- Occasional **microstutters**, especially with heavier graphics settings or texture packs.
- The new installer, fresh-boot native saves, gyro, adaptive L2 and controller speaker still need wider PS5 testing. These controller extras are optional and start off.
- No HDR. Online, mods, Controller Paks and other firmware/TV combinations need broader testing. 120-Hz output is not a promise of constant 120 FPS.

Next work focuses on these fixes and compatibility testing. Gyro and adaptive L2
are already implemented, not future-only features. [Known issues](docs/KNOWN_ISSUES.md).

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
