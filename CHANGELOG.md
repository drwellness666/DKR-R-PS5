# Changelog

## 1.0 — in preparation

- Read controller-mapping imports completely before applying them, with bounded USB input outside the input lock.
- Keep existing controller-mapping exports if the destination changes after file selection.
- Finish or cancel texture imports cleanly when closing DKR-R or installing an update.
- Keep tiled CRT filters lightweight on PS5 by repeating their texture on one quad.
- Browse USB drives and game files directly in the touchpad menu when importing textures, filters, saves or controller mappings. Export saves and mappings to a chosen folder; existing file names are kept.

First PS5 release, based on **DKR-R 1.0.8**. The 1.0.8 Windows startup-memory hotfix is included; the native PS5 memory path is unchanged. Includes its release-source layers for SDK 2, custom cups, the mod library, shared offline saves across mods, camera, water, presentation and performance fixes.

- Native PS5 home-screen app with original DKR-R artwork.
- Both US revisions in one app, shared settings and textures, separate progress.
- Controller-friendly settings, readable 4K menus and a live button-layout diagram.
- Classic driving controls and optional DualSense balloon colors, gyro, adaptive L2 and controller sounds.
- Controller effects can be retried by toggling their option after a temporary output failure.
- Optional 4K and HDR10 output; experimental 120 Hz with VRR.
- One ROM-free installer using the owner's USB ROMs, native saves, in-game Internet updates and a manual update ELF.
- Update recovery keeps its backup intact when directory access fails.
- English and German player guides, linked credits and complete original component notices.
- The optional PC build assistant creates the same ROM-free installer and update files, verifies completed builds before reuse and updates without a previous PC build.

[Known issues and remaining feature checks](docs/KNOWN_ISSUES.md) · [Full credits](CREDITS.md)
