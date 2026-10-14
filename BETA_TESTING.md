# Beta testing the native PS5 source preview

This is an experimental port built locally from a supported, legally usable dump.
There is no public prebuilt game download. Read `GAME_BUILD.md` and
`LICENSE_REVIEW.md` before building or distributing anything.

## Identify the test

Record the integration Git commit, upstream version (currently 1.0.6), selected
ROM revision/canonical hash, host compiler/CMake versions, PS5 firmware, and exact
loader/kstuff/ShadowMount versions when known. An ELF transfer message proves only
transfer. A successful present call proves the API returned success, not that the
picture, text and controls are correct.

The PS5 title ID `PPSA94678` is a local preview choice, not a reserved worldwide
identifier. Check for an installed title/path conflict before installation.
The installers refuse different files; do not bypass that check or overwrite a
foreign application to force this preview to start.

## First acceptance pass

1. Start the native title. Confirm the intro/title screen is visible, sound is
   audible and Cross/Options respond. Note exact errors and whether the console
   menu remains usable.
2. Check ordinary track selection outside Adventure, the Adventure intro and
   save-slot names. Earlier labels were repaired; qualify the newest build and
   distinguish the initial water-preview flash from sustained rendering defects.
3. Drive one complete race. Check HUD/text, steering, acceleration/braking,
   music/effects and whether the game progresses normally.
4. Open settings with Touchpad. Check readable headings/body text, L1/R1 pages,
   vertical D-pad order, scrolling, Cross activation and Circle/Touchpad return.
5. With the read-only save-metadata helper running, create/save a test progress
   point in the port's own profile. Exit normally, start again and verify the
   same progress. Preserve existing data if a step fails. A successful write
   message is not enough to claim save/load acceptance.
6. Check Controller Pak persistence separately when a game feature uses it.
   Enable Memory Pak and Rumble Pak independently and together. Use Controls'
   Identify with vibration, then check in-race vibration and that it stops after exit.
7. Check return to the PS5 menu, focus changes and a second launch. Do not infer
   this from the earlier synthetic GPU/RT64 probe.

Each item has its own result: PASS, FAIL or NOT TESTED. Leave untested items
explicitly untested. The new US-dual/three-language candidate still needs this
console acceptance pass.

Check title icon and selected-title background after the loader's normal artwork
refresh. Confirm that the matching launch image releases when the first game frame
completes. A visible launch image does not prove the selected-title background. With a backed-up test save, select German using the original game
**Options → Language** control; verify left/right cycling in both US revisions and check menu/track names, dialogues, accents and that
the selection survives a normal save/relaunch. Use `CONSOLE_FEATURES.md` for the
exact language/Pak scope; PAL engines are currently unsupported.

For the reported water flicker, enter Tracks mode from a fresh process, observe
the first seconds of a water track's preview and compare later stable frames.
Repeat the same track before changing settings. Separate Accurate/Modern,
original/replacement textures and cold/warm entry. `RENDER_ISSUES.md` documents
opt-in measurements and individual water baseline switches. Do not declare a
fix solely because the flicker happens to settle later.

## Stutters and optional texture packs

First establish an unmodified baseline without external replacements. Then
repeat the same scene, resolution and presentation settings with a pack you have
separately obtained under its author's terms. Record cold-start versus repeated
scene behavior and any changed setting. Note whether a hitch accompanies a new
track, menu opening, texture loading or a save. Do not label shader compilation
as the cause without supporting timing evidence.

Rice-format packs are handled through DKR-R's own importer; filenames alone do
not prove complete visual compatibility. Replacements need the Modern profile.
Do not attach a texture pack to a GitHub issue or release. The private SR.GU test
package is not part of this repository.

## What to include in a report

Use the GitHub bug template. Include the smallest reproducible sequence and a
short filtered DKR-R log excerpt. Source/build failures should name the failed
stage and the relevant compiler/linker error. Exclude ROMs, generated CPU files,
ROM-bearing installers, game binaries, profiles/saves, private network addresses,
full console dumps, credentials and unrelated Sony-module data.

An issue report must not claim broader firmware compatibility, stable frame rate
or storage safety than the reported test demonstrates.

## Optional output comparison

Follow [DISPLAY_FEATURES.md](DISPLAY_FEATURES.md). First retain the accepted 1080p/60 baseline; then apply 4K/60 for the next start while keeping the same game rendering/detail and scene. Record picture, UI, sound, hitch timing and actual TV HDMI information. Restore 1080p/60 before comparing higher internal render detail. The capable 120-Hz metadata bundle is a separate later experiment and is not eligible for the ordinary metadata-preserving update. Compare VRR/ALLM as system settings only after stable picture/audio. Native HDR is unavailable; record system SDR conversion separately.
