# Updates

## From the game

Open **Updates** in the launcher, or **Touchpad → Updates** during a game, and choose **Check for updates**. When a compatible release is available, choose **Install update and close DKR-R**. Wait for the completion notification, then start DKR-R again from its PS5 icon.

The game downloads and verifies the update itself. **No PC, USB drive, ELF receiver or background helper is required.** Automatic checks at launch are optional and off by default; installation waits for your choice.

Updates keep your ROMs, settings, progress, Paks and texture packs. Back up important saves before updating. If an in-game update is interrupted, recovery runs at the next launch; follow any restart notification.

## Automatic storage cleanup

DKR-R 1.0.15 automatically tidies old downloads and caches once per application start, on a background worker. Installed mods, texture packs, ROMs, saves and current recovery files are kept. Under **About DKR-R → Support → Storage**, you can inspect storage, run cleanup manually or disable automatic cleanup. Crash dumps older than 30 days are only removed through their separate explicit option.

## Manual or offline update

Close DKR-R and send **DKRR-Update.elf** to the console's ELF receiver. Wait for its completion notification, then start the game again. Your installed ROMs are reused. Rerun the same ELF if a manual update is interrupted.

## Common questions

- **No update available?** Only published, compatible releases appear. A source-code change on GitHub is not an installable update.
- **Private repository?** In-game downloads need public GitHub access. This version does not store GitHub credentials.
- **Not enough space?** Free space for the download and recovery copies, then retry. Do not remove save folders.
- **What is `DKRR-Update.dkru`?** The update data downloaded by the game. You do not need to open or send it yourself.

[Install](INSTALLER.md) · [Save data](SAVES.md) · [Known issues](KNOWN_ISSUES.md)
