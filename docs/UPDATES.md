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

Upgrading from Beta 3.3 or earlier? Install the new **DKRR-Update.elf once** to
add the native installer and prepare access to DKR-R's own program folder.
Subsequent Internet updates happen entirely inside the game.

Downloads require this project's public GitHub releases. A private repository
is unavailable to the game; no GitHub credentials are stored on the PS5.
[Current test status](VALIDATION.md).

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

**DKRR-Update.dkru** is the signed data package downloaded by the game. It is not
an ELF and does not execute downloaded installer code. **DKRR-Network-Update.elf**
is kept only for older Beta 3.3 clients; new clients never use it.

[Install](INSTALLER.md) · [Technical design](ARCHITECTURE.md).
