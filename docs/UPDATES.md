# Update the game

1. Download the new **DKRR-Update.elf** from this project's release.
2. Close DKR-R and start the normal homebrew / ELF receiver environment.
3. Send the update ELF once. Wait for completion, then start the PS5 icon.

The update reuses the ROMs already on the console. Controls, picture/sound
settings, progress, Paks and texture packs are preserved. No compiler, old PC
build or always-running PC is required. Missing ROMs can be supplied again in
`USB/DKRR`.

Program files are prepared and checked before replacement. Previous program
versions are kept for recovery; this uses extra internal storage. If interrupted,
rerun the ELF before launching. Unknown or modified app files and accidental
downgrades are refused. Early unmanaged previews are recognized only when their
exact version was included in the updater's migration list.

`DKRR UNIVERSAL UPDATE PASS.` is the receiver's completion marker. An ELF
transfer alone does not mean the installation completed. There is no silent
Internet download or automatic background installation.

The universal update is part of the beta. [Install](INSTALLER.md) ·
[Test status](VALIDATION.md) · [Technical design](ARCHITECTURE.md).
