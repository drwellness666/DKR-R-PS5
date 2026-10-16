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

Normal vibration is available. **Controls → Device → Identify with vibration**
plays a short test pulse. Optional **Match held balloon color** matches the
controller light to the held item and returns lighting to the system when no
item is held. It starts disabled. Gyro and adaptive triggers are [planned](ROADMAP.md).

## Choose the game revision and language

**Touchpad → Play → Game version** selects US 1.0 or US 1.1 for the next launch.
Close the game normally and start it again; **Playing** shows the active version.
A single-ROM build offers only its included version.

The original game's **Options → Language** cycles English, French and German.
This uses the text already present in the supported US dumps; a PAL ROM is not
needed. The Recompiled settings interface is English.

## Shared settings, separate progress

Both revisions share controls, graphics, audio, HUD and texture selection.
Adventure progress and Controller Paks remain separate per revision. See
[save data and startup support](SAVES.md) for paths and restart behavior.
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
