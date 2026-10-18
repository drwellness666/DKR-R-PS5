# Known issues and compatibility ⚠️

## Known problems

| Problem | Details |
|---|---|
| Tracks-preview water | A brief flash can appear when entering a course preview. It settles afterward; Adventure water was reported stable. |
| Microstutters | Occasional short stutters remain. High render resolution, anti-aliasing and texture packs add load. |
| Adventure intro | A new-game intro skip was reported but has not been reproduced consistently. |

## Feature status

**Version 1.0 is in preparation.** Earlier PS5 sessions confirmed gameplay picture/sound, both US revisions, save/reload, shared settings/textures, classic controls, vibration, balloon light colors, readable 4K menus and title artwork. Output at 4K/119.88 Hz with VRR Automatic was confirmed on an LG C1.

Those sessions used **firmware 13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite and ShadowMountPlus**. This is compatibility evidence, not a requirement to update your firmware.

The combined version still needs these final PS5 checks:

- Fresh installation and save/load after a full console restart.
- In-game Internet update, restart and interrupted-update recovery.
- Gyro, adaptive L2 and controller-speaker behavior.
- HDR brightness, SDR fallback and HDR with VRR/120 Hz.

The features are implemented and have automated checks; those do not establish physical behavior on every console. Controller sounds are short cues, not the full game's spatial audio mix. Normal TV sound stays available.

Online play, arbitrary mods, ghosts/Paks, other loaders/displays and the complete Windows/WSL2 source-build route have limited coverage. Constant 120 rendered FPS is not guaranteed.

[Report a problem](https://github.com/drwellness666/DKR-R-PS5/issues) · [What to include](DIAGNOSTICS.md) · [Planned improvements](ROADMAP.md)
