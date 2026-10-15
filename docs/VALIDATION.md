# Validation scope

## Source checks

The release preparation passed all **43 commands** in the exported
`.github/workflows/source-check.yml`. This includes the source/privacy scan,
controller input and light policies, shared profile/save routes, display handling,
actual ImGui menu geometry, diagnostics, installers and guarded updates.
The framebuffer test used a freshly prepared dependency tree after the archive
inventory changed; a stale fixture was correctly refused before that rerun passed.

Fresh dependency preparation verifies pinned inputs, applies all 54 dependency
patches and checks the saved preparation stamp. Both US language bridges and
shared texture/configuration routes have ROM-free host checks. The local build
assistant has HTTP access checks, ROM rejection tests, single/dual build recipe
checks and installation success-marker tests. Its browser view and PC prerequisite
check were also exercised. The complete assistant-driven game build and installation
have not yet received an end-to-end console test; it uses the existing native build
and installer recipes.

## Physical PS5 results

The accepted native game build is `integrated-8f7c534368169906167740cc`.
Its installed executable was read back with matching SHA-256:
`4b0990c72a40742b4324182907631a13cdac3526489653b41efde47b6a055fb3`.
The user confirmed:

- Normal gameplay image and audio, readable save names and full-size menus at 4K.
- US 1.0 and US 1.1 selection, save/reload and shared settings/texture selection.
- Classic Cross acceleration, gas tapping and L2 items/horn; vibration.
- English/French/German language selection.
- Balloon light colors and return to normal system lighting without an item.
- Version-card edges aligned with Return to the race.
- Icon, home-menu cloud background and matching launch artwork; the launch overlay clears.
- 4K/119.88-Hz output with VRR Automatic, unsupported-games override Off and VRR active on LG C1.

PS5 AV output reported HDMI2160P11988, VRR48–120 and ALLM game. This is output
negotiation evidence, not a guarantee of 120 rendered FPS throughout the game.
Tested firmware/loader details are in the [manual build guide](BUILD.md).

Brief Tracks-preview water flicker and occasional microstutters remain reported.
A clean 60-second frame-time sample cannot cover every scene and graphics setting.
Online play, arbitrary mods, dedicated ghost/Pak persistence and other displays or
loaders still need wider testing. See [known issues](KNOWN_ISSUES.md).

## Reproduction

Use `.github/workflows/source-check.yml` for the ROM-free host gate. Hardware
acceptance remains separate: source tests, a successful link or an installer
receipt alone cannot establish visible output, physical controller behavior or
sustained performance. Raw console logs and ROM-derived debug symbols stay private.
