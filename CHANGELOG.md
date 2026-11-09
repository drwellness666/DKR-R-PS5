# Changelog

## 1.2 — 2026-11-09

Update to **DKR-R 1.0.15**. Installed 1.0 and 1.1 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. Online lobbies need matching 1.0.15 games on every player.

- Launcher first: starting DKR-R from the PS5 home screen opens the launcher. Choose the version, mods and settings, then press **Start Game**. Stopping a race returns to the launcher; the "Skip launcher" option is no longer offered on PS5.
- New from DKR-R 1.0.15: smoother online recovery after delays, live host settings (prediction window and input delay during a race), better spectators, Start Game kept at the bottom of Play, character names in Player Select and automatic storage cleanup of old downloads and caches (About DKR-R → Support → Storage; saves, ROMs, installed mods and texture packs are kept).
- Launcher text: hyphens, slashes, brackets and counts now sit on the same line as the letters ("D-PAD" no longer looks like "D_PAD"); digits match the letter height, also in button names such as L1/R2.
- Controls: PS5 button names in Title Case ("D-Pad Up", "Left Stick Up") and the controller shown as "DualSense (PS5)". The sidebar navigation switch is removed; L1/R1 change pages and the D-pad moves inside a page.
- In-game updater: cancelling no longer leaves it stuck on "Downloading"/"Checking", slow but working connections no longer fail after 10 minutes, and clearer error messages.

## 1.1 — 2026-11-07

Update to **DKR-R 1.0.14**. Installed 1.0 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept.

- New from DKR-R 1.0.14: Language choice in **Play** (Automatic follows the PS5 system language; some translations are drafts), aspect-specific render resolutions, a Session mods switch, Skip launcher option and the new Starting Game screen.
- Matched launcher and in-game overlay pages, consistent Search/Status/Sort across the Mod Library, sidebar navigation by LB/RB or D-pad and improved stick navigation.
- Custom-car and road-wheel lighting fixes, seasonal themes (on PS5 using the global calendar) and optional crash reports that only start after your consent.
- PS5: the PC child-process updater stays excluded, so updates continue through the signed PS5 package; online play uses the current DKR-R 1.0.14 protocol.
- Fixed the in-game updater: 1.0 could report "Download cancelled or verification failed" after a complete, correct download, because its package check was built with a mismatched Mbed TLS configuration. 1.1 checks downloads correctly, and the 1.1 package is laid out so that the installed 1.0 accepts it.

## 1.0 — 2026-10-31

First native PS5 release, based on **DKR-R 1.0.9**.

- One home-screen app with both US revisions, shared settings/textures and separate progress.
- Original DKR-R artwork, controller-friendly mapping and optional adjustable analog-stick menu pointer with right-stick scrolling.
- Optional DualSense balloon colors, stable gyro steering, adaptive L2 item feedback and original game sounds on the controller speaker.
- Optional 4K, neutral HDR10 and 120-Hz output with VRR; Modern rendering follows the display rate.
- Native networking for the Mod Store, online directory/lobbies and Internet updates, using the system DNS resolver.
- A ROM-free USB installer, an optional offline update ELF and signed in-game updates that keep installed data.
- Correct native JSON locale handling for custom water/track metadata and save-record handling on subsequent starts.
- Remove accumulated software draw commands from GPU menus; move large import scans out of the UI lock and diagnostic writes off rendering threads.
- Let healthy large downloads continue; reduce repeated texture-copy barriers and redundant hashing while retaining verification and recovery.
- English and German player guides, upstream credits and complete original component notices.

[Known issues and compatibility](docs/KNOWN_ISSUES.md) · [Full credits](CREDITS.md)

- Holding Square no longer opens the desktop window selector; Square actions in custom-cup widgets remain available, and conflicting menu events no longer build up.
- Controller horns use the admitted audio bank of installed custom drivers automatically.
