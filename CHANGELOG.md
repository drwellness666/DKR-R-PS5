# Changelog

## 1.2.3 — 2026-11-11

Corrects a mistake in 1.2.2. Installed 1.0, 1.1, 1.2, 1.2.1 and 1.2.2 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. This change was checked on a PC and still needs its own test on the console.

- **Online:** 1.2.2 made the Touchpad button open lobby chat during an online race. Touchpad also opens and closes the DKR-R menu, so both could happen at once. That is removed. The chat button is now your choice: **Online → Overlays → Lobby chat → Chat button**. It is off by default, and a button that already opens the menu, runs a shortcut or drives the kart is refused. **Clear chat button** turns it off again.

## 1.2.2 — 2026-11-10

Fixes for problems found on the PS5 in 1.2.1. Installed 1.0, 1.1, 1.2 and 1.2.1 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. These fixes were checked on a PC and still need their own test on the console.

- Signing in with Discord now stores the sign-in token correctly.
- **Online:** in a lobby, the Touchpad button now opens chat even without a connected keyboard; a hint next to the chat explains this.
- **Graphics:** the Texture Diagnostics list no longer runs past the edge of the page; it scrolls in its own area now.
- **Controls:** "D-pad" is written "D-Pad" everywhere on the PS5, and the vehicle rows no longer overlap the stick icons.
- **About DKR-R:** the release-info block that duplicated the Updates page is hidden on the PS5. Extra spacing around the crash-reporting section avoids an overlap with the storage buttons.
- Not fixed: a reported shadow difference between the My Mods and Magic Codes cards could not be reproduced as a code difference; no change was made.
- Still open: menus can stutter briefly when scrolling up, and water can flicker the first time it appears. See the [known issues](docs/KNOWN_ISSUES.md).

## 1.2.1 — 2026-11-10

Fixes for problems found in 1.2. Installed 1.0, 1.1 and 1.2 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept.

- The crash-report question is asked only once. Before, your answer was not saved and the question came back at every start.
- **Keep me signed in** now works on the PS5: sign in to Online once and you stay signed in the next time you start. Signing out removes the saved sign-in.
- **Play:** the Ready to Race panel and its traffic light are centred, and Modded Adventure no longer touches the text above it. A new **Update available** button appears when an update is waiting.
- **Online:** the pencil button next to the lobby name and the decorative squares on the lobby cards are adjusted. The squares on the Magic Code cards got the same fix.
- **Updates:** the page now shows which step is running (checking, downloading, verifying, installing), with megabytes and percent.
- **About DKR-R:** shows the PS5 port version with links to its GitHub page and release notes. **Storage and reports** is back: there you can turn off the automatic cleanup and set the crash-report choice. It was hidden on the PS5 in 1.2.
- **Graphics:** the presets are now **Performance** (1080p, up to 120 FPS) and **Quality** (4K, up to 120 FPS). Applying one also sets the TV output, which takes effect the next time you start the game.
- Cleaner button names in the controller setup and the HUD layout editor (Cross, Circle, Triangle, L1/R1, Options).
- Not fixed yet: menus can stutter briefly when scrolling up, and water can flicker the first time it appears. See the [known issues](docs/KNOWN_ISSUES.md).

## 1.2 — 2026-11-09

Update to **DKR-R 1.0.15**. Installed 1.0 and 1.1 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. Online lobbies need matching 1.0.15 games on every player's side.

- **Launcher first:** DKR-R opens its launcher when you start it from the PS5 home screen. Choose the version, mods and settings, then press **Start Game**. Stopping a race returns you to the launcher. **Skip launcher** is no longer offered on the PS5.
- New from DKR-R 1.0.15: smoother online recovery after delays, host settings you can change during a race, better spectating, Start Game kept at the bottom of Play, character names in Player Select, and automatic cleanup of old downloads and caches. Saves, ROMs, installed mods and texture packs are never touched.
- Clearer launcher text: dashes, slashes, brackets and counts line up with the letters ("D-PAD" no longer looks like "D_PAD").
- Controls: PS5 button names ("D-Pad Up", "Left Stick Up") and the controller shown as "DualSense (PS5)". The sidebar navigation switch is gone: L1/R1 change pages and the D-pad moves inside a page.
- The in-game updater no longer gets stuck when you cancel, slow connections no longer fail after 10 minutes, and error messages are clearer.

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
