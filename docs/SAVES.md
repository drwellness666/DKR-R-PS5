# Settings and save data

**Settings and textures are shared; progress and Paks stay separate.** Switching
between US 1.0 and 1.1 does not convert or overwrite the other revision's progress.

| Data | Internal storage |
|---|---|
| Shared controls, graphics, audio, HUD and texture library | `/data/dkrr/profile-us-v77` |
| US 1.0 progress and Controller Paks | Existing save folders inside `profile-us-v77` |
| US 1.1 progress and Controller Paks | Existing save folders inside `profile-us-v80` |
| Owned ROMs | `/data/dkrr/roms` |

These names match the upstream profile layout. Do not rename the folders to
switch game revision; use **Touchpad → Play** instead. Recompiled does not
provide a shared emulator-style save-state file across the two revisions.

The current version handles saving inside the game. No PC service or additional save-helper
ELF is part of this version. Saves are checked and written through temporary
files with backups retained. Installers and updates do not replace save data.

Save/reload was confirmed on both revisions in earlier sessions. The new native
backend passes host failure tests; a full console restart followed by save/load
with the new installation remains to verify. [Test status](VALIDATION.md).
