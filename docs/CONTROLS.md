# Controls, versions and texture packs

[Deutsch / Einstieg](../README.de.md) · [Save data](SAVES.md) · [Display](DISPLAY.md)

## DualSense

| Button | Action |
|---|---|
| Cross | Accelerate; tapping works as in the original game |
| Square | Brake / reverse |
| Left stick / D-pad | Steering / selection |
| R1 | Drift / original R button |
| L2 | Item / horn / original Z button |
| Options | Original Start button |
| Touchpad | Open or close Recompiled settings |

In settings, L1/R1 change pages, D-pad/left stick move selection, Cross confirms
and Circle goes back. **Controls → Button mapping** supports custom bindings.
The old trigger-driving preset was removed; custom mappings are preserved.

Normal vibration is available under **Controls → Device**. PS5-specific options
are together under **Controls → PS5**:

- **Match held balloon color:** the held item color, then normal system lighting.
- **L2 missile and trap feedback:** light resistance and an item-use impulse, including release-to-fire.
- **Original balloon item sounds:** optional short boost, missile, trap, shield and magnet cues on the controller speaker. Volume is adjustable; normal TV sound stays active. PCM is generated privately from your ROM.
- **Gyro steering:** enable per player, choose roll/yaw, sensitivity, deadzone, calibrate and recenter. Requires Modern presentation; stick steering remains available.

All optional effects start off and save with the shared configuration. The new
native gyro, adaptive-trigger and speaker paths are experimental and await broader PS5 testing.
The speaker uses an experimental native PADSPK route; unavailable output is
reported in this page without rerouting or stopping TV audio.

## Choose the game revision and language

**Touchpad → Play → Game version** selects US 1.0 or US 1.1 for the next launch.
Close the game normally and start it again; **Playing** shows the active version.
The normal installer includes both supported revisions.

The original game's **Options → Language** cycles English, French and German.
This uses the text already present in the supported US dumps; a PAL ROM is not
needed. The Recompiled settings interface is English.

## Shared settings, separate progress

Both revisions share controls, graphics, audio, HUD and texture selection.
Adventure progress and Controller Paks remain separate per revision. See
[save data](SAVES.md) for paths and restart behavior.
Virtual Memory Pak and Rumble Pak can be enabled together. Adventure save/reload
and physical vibration were confirmed; dedicated ghost/Pak persistence needs
more testing.

## Texture packs and mods

Obtain compatible packs separately under the author's terms. Use DKR-R's existing
**Textures** page and importer. Replacements require the **Modern** presentation
profile. Keep the imported archive in the game's texture library; a selected
archive is shared between US 1.0 and US 1.1.

Existing US 1.1 archives can be discovered without duplicating them; a same-name
entry in the common US 1.0 library takes precedence. The tested Remastered pack
is not bundled or uploaded by this project. Arbitrary packs, mods and online
play have not all been tested on PS5.
