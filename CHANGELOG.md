# Changelog

## Beta 3.1 — console menu refinements

- Put Controls, Graphics, Sound and Textures next to Play; preserve existing settings and page identities.
- Replace overflowing horizontal subsection tabs with one full-width section selector, using Cross to open/select and Circle to cancel.
- Separate Graphics into Display and HUD & Detail; simplify the paused-race page.
- Reuse loaded controller-sound cues and the prepared missile mix instead of reading/mixing them again on every use.
- Store identical title images only once inside each installer/update ELF; installed artwork is unchanged.
- Recommend the optional DKR REMASTERED pack by sr.gu, with its official community link.

This candidate does not claim a fix for Tracks-preview water flicker or measured
FPS gains. The published Beta 3 package remains available unchanged.

## Beta 3 — universal installation and native PS5 extras

- One ROM-free installer ELF: import both owned US ROMs from USB on PS5.
- One update ELF: retain ROMs, settings, progress, Paks and texture packs; no previous PC build needed.
- Verified staging, release-version checks, retained recovery copies and interrupted-update recovery.
- Direct native saves; remove the old preview's separate save-metadata helper.
- Controls → PS5: optional balloon light colors, gyro steering, adaptive L2 item feedback and controller sounds for all five balloon types.
- Generate controller audio locally from the owner's ROMs; shared ELFs exclude those samples.
- Match display as the default Modern rendering target, following native 60/120-Hz output.
- English main README with German translation, shorter player guides and a clear PS5/upstream feature comparison.

The new install/save/controller paths are experimental. Native builds and 53
host check commands pass; their full console verification remains open.

## Beta 2.1

Shorter documentation, Windows/WSL2 instructions and clarification of the old
save-support startup dependency. Gameplay code unchanged from Beta 2.

## Beta 2

Dual-US selection, shared settings/textures with separate progress, original
EN/FR/DE game text, classic controls, balloon lights, 4K menu scaling, experimental
120-Hz/VRR output, PS5 title artwork and local diagnostics.

Known issues remain: brief Tracks-preview water flicker and occasional
microstutters. [Details](docs/KNOWN_ISSUES.md).
