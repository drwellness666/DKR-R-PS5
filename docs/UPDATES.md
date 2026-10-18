# Update the game

## In the game

Open **Touchpad → About DKR-R → PS5 updates** and choose **Check for updates**.
Choose **Install update and close DKR-R** when a newer release is available.
The game downloads and verifies the package, saves and closes, then replaces
its own program files. Wait for the completion notification and start its PS5
icon again.

**No ELF receiver, PC, USB drive or background helper is needed for this route.**
Optional launch checks are off by default. Installing an update always waits
for your choice. Your usual jailbreak/homebrew environment is still required
to run the game after a console restart.

Downloads require this project's public GitHub releases. A private repository
is unavailable to the game; no GitHub credentials are stored on the PS5.
[Current test status](VALIDATION.md).

## First release

Version 1.0 is the first public installation baseline. Private development
previews do not need a migration path. Updates described here apply to compatible
installations made with the 1.0 installer or a later supported version.

## Manual or offline update

Close DKR-R and send **DKRR-Update.elf** using the console's normal ELF receiver.
Wait for its completion notification, then start DKR-R. This option works
without Internet access on the console. Existing ROMs are reused.

Both routes preserve controls, picture/sound settings, progress, Paks and texture
packs. New in-game updates also retain the controller sounds already generated
from your ROMs. An interrupted in-game update is recovered automatically when
DKR-R next starts. If it restores the previous version, a notification asks you
to start the game again. An interrupted manual ELF installation is recovered by
rerunning that ELF. Do not delete save folders.

The update card shows estimated free space needed, including recovery copies.
A measured shortage stops before downloading. If the console cannot report free
space, the card says so; ordinary verification and recovery still apply.

**DKRR-Update.dkru** is the signed data package downloaded by the game. It is not
an ELF and does not execute downloaded installer code. No separate network
updater ELF is included in the 1.0 downloads.

[Install](INSTALLER.md) · [Technical design](ARCHITECTURE.md).
