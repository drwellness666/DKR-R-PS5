# Update the game

## From DKR-R on the PS5

Open **Touchpad → About DKR-R → PS5 updates** and choose **Check for updates**.
When an update is available, **Install update and close DKR-R** downloads it,
verifies it and closes the game before installation. Wait for the PS5 completion
notification, then start DKR-R from its icon again.

The normal ELF receiver must already be running on the PS5. No PC, USB drive or
extra manually started helper is needed. **Check for updates at launch** is
optional and off by default; installation always waits for your choice.

Internet updates use this project's public GitHub releases. A private repository
cannot be accessed by the game; no GitHub credentials are stored in it. This new
path is an experimental candidate; its exact test scope is in [Test status](VALIDATION.md).

## With an update ELF

Close DKR-R and send the release's **DKRR-Update.elf** to the usual ELF receiver.
Wait for the completion notification, then start the PS5 icon. This route also
works when the console cannot reach GitHub. Missing ROMs can be supplied again in
`USB/DKRR`.

Both methods reuse the console's ROMs and preserve controls, picture/sound
settings, progress, Paks and texture packs. Program files are prepared and
checked before replacement; previous versions are kept for recovery and use
extra internal storage. Rerun the manual update ELF after an interrupted
installation. Unknown app files and accidental downgrades are refused.

**DKRR-Network-Update.elf** is the in-game client's download. It waits for the
client's commit and must not be sent as a manual update. Use **DKRR-Update.elf**
for that.

[Install](INSTALLER.md) · [Technical design](ARCHITECTURE.md).
