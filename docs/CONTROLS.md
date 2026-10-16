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
and Circle goes back. Common settings come first: **Play → Controls → Graphics
→ Sound → Textures → Save Manager**, followed by Mods / Hacks, Online and About.
Use the full-width **SECTION** selector to choose a page's subsection with Cross
and D-pad; Circle cancels. **Graphics** separates Display from HUD & Detail.
**Controls → Button mapping** shows the current player’s controller layout. Choose
an action, then **Change** to bind a button or trigger. **More binding options**
contains alternate bindings, Classic restore and copying Player 1’s layout.
Restoring a layout can be undone.
The old trigger-driving preset was removed; custom mappings are preserved.

Normal vibration is available under **Controls → Device**. PS5-specific options
are together under **Controls → PS5**:

- **Match held balloon color:** the held item color, then normal system lighting.
- **Adaptive L2 item feedback:** light resistance and an item-use impulse, including release-to-fire.
- **Controller sounds:** one switch for all short original cues: all five item types, weapon-balloon pickups/upgrades, silver coins, gold balloons, your character’s horn and boost pads. Set their volume with **Controller volume**; TV sound stays active.
- **Gyro steering:** enable per player, choose roll/yaw, sensitivity, deadzone, calibrate and recenter. Requires Modern presentation; stick steering remains available.

All optional effects start off and save with the shared configuration. Sounds go
to the local player’s controller, with item use taking priority and no growing
sound queue. They stop while the game is paused or loading. The installer
prepares them from your ROMs; no game audio is distributed. Gyro tuning stays
under **Gyro tuning**. [Feature status](VALIDATION.md).

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

**Optional recommendation: DKR REMASTERED by sr.gu.** Version **0.2.7** was
confirmed on both US revisions during PS5 sessions. Upstream directs players to
the [official DKR-R Discord](https://discord.com/invite/AMWfXdBjNP) for its
community texture pack; follow the author's current download there. See also
[upstream texture-pack help](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/TEXTURE_PACKS.md).

Obtain compatible packs separately under the author's terms. Use DKR-R's existing
**Textures** page and importer. Replacements require the **Modern** presentation
profile. Keep the imported archive in the game's texture library; a selected
archive is shared between US 1.0 and US 1.1.

Existing US 1.1 archives can be discovered without duplicating them; a same-name
entry in the common US 1.0 library takes precedence. The tested Remastered pack
is not bundled or uploaded by this project. Arbitrary packs, mods and online
play have not all been tested on PS5.
