# Changelog

## 1.2.9 — 2026-11-12

Fix for a problem found on the PS5 in 1.2.8. Checked on a PC; still needs its own test on the console.

- **Could not create a lobby after a rejected join:** a connection that failed to join or spectate (for example "Game Pak revisions do not match") stayed in that failed state until the app restarted. **Start a lobby** looked clickable, but every attempt was instantly cancelled with "Preparation cancelled because the lobby, ROM or Magic Codes changed." even though nothing had changed. Starting a lobby now clears a leftover failed connection first instead of refusing to host. The same stale-state check was hardened in the related mod-preparation code path.

## 1.2.8 — 2026-11-12

Fixes for what was found on the PS5 in 1.2.5. This also contains everything of internal 1.2.6 and 1.2.7 builds. Installed 1.0 to 1.2.7 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. These fixes were checked on a PC and still need their own test on the console.

- **Lobby chat button:** the launcher's own input loop never passed controller buttons to the chat capture, so no button was ever registered there. It does now; press a free button within ten seconds.
- **Online:** the notice "The directory connection failed. Retrying..." is removed as soon as the connection is open again.
- **Controls → PS5:** the groups are separated by a thin line, like on the other pages; the sound choice stays with its group, without its own line.
- **Join and storage evidence:** a rejected online join now logs this device's and the host's Game Pak revision and ROM hash; the storage cleanup records every file it removes (or would remove) in the journal.
- **Fault evidence:** the game now writes a readable record of what it was doing when it stops unexpectedly (`diagnostics/run-*.log`), plus `logs/selftest.json` with what the console allows. The new `tools/pull_console_debug.py` reads both over FTP, read-only, and names the function and line of a crash. See `docs/DEBUGGING.md` and `docs/PS5_FACTS.md`.
- **Cause of the storage and first-start crashes, confirmed:** the PS5 does not provide `readlink` (nor `symlink`, `link`, `fchown`, `mkstemp`, `strcasestr`); a call jumps to address 0. The storage cleanup read the install folder through `readlink`, and the new self-test hit the same call at the first start. These six functions now exist in the game and fail with an error instead of crashing; the build checks every import against a recorded list (`docs/IMPORTS_BASELINE.json`).

## 1.2.5 — 2026-11-11

Fix for 1.2.4 closing with an error about ten seconds after it starts. This fix was checked on a PC and still needs its own test on the console.

- **Crash after start:** 1.2.4 was the first version in which the storage tools could run on the PS5, and the game closed just as the automatic cleanup began, ten seconds after launch. The exact fault inside the cleanup is not known yet. Version 1.2.5 switched the automatic cleanup off; 1.2.6 has it on again, with a safeguard: if a cleanup does not finish, the next launch skips it once.
- **Storage and reports:** Check Storage, Clean and the cache buttons stay available. They may hit the same fault. Each step is now written to `logs/storage-trace.log` in the game's data folder, so a report can name the step that failed.
- **Updating from 1.2.4:** if the game closes before you reach **Touchpad → Updates**, send `DKRR-Update.elf` to the console the same way as the installer. Settings, saves and texture packs are kept. Versions 1.0 to 1.2.3 update in game as before.

## 1.2.4 — 2026-11-11

Fixes for problems found on the PS5 in 1.2.3. Installed 1.0 to 1.2.3 games update in game under **Touchpad → Updates**; settings, saves and texture packs are kept. These fixes were checked on a PC and still need their own test on the console.

- **Saving settings:** the crash-report choice, **Keep me signed in** and the **Storage and reports** page all failed for one reason. DKR-R needs a temporary folder for the safety lock it takes before writing them. Since 1.2.1 it tried to create that folder in `/data/dkrr`, which belongs to the system and cannot be written by the game, so the folder never existed. It now lives inside the game's own profile folder. The saved sign-in token also uses plain file calls now, and a refused flush to disk no longer throws a saved choice away. If anything still fails, the reason is written to the game log.
- **Storage and reports:** with the lock working, Check Storage, the automatic cleanup and the cache buttons can run.
- **Lobby chat button:** the PS5 now shows one control (the old keyboard-key button is hidden). Press a free button within ten seconds; if the button is already used, the page says what it does. Circle is free by default.
- **Magic Codes:** the dark squares that looked like a shadow above each card are gone, and long names such as OppositesAttract shrink to fit on one line.
- **Controls → PS5:** the settings are grouped under Menu navigation, DualSense feedback, Controller sounds and Motion steering, with even spacing.
- **Graphics:** the closed Texture Diagnostics header is compact again.

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
