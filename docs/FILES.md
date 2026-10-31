# Which download do I need?

| File | Purpose |
|---|---|
| **DKRR-Installer.elf** | First installation; import both owned US ROMs from USB |
| **DKRR-Update.elf** | Optional manual/offline update; keep installed ROMs and saves |
| DKRR-Update.dkru | Data downloaded by the in-game updater; no manual action needed |
| DKR-R-PS5-source.zip | Matching source for rebuilding or contributing |
| SHA256SUMS.txt / BUILD-INFO.json | Download checksums and version information |

Normal installation needs only the installer ELF and your ROMs. Shared installers contain program code and interface artwork, with no ROMs or extracted controller audio.

The repository contains source, build recipes and required licences. Personal ROMs, saves, packs, logs, keys and local build/audit folders stay outside it.


Import and export buttons open a file browser inside the game. Choose your USB drive or `/data/dkrr`, then the file or destination folder. Cross selects; Circle cancels. Only matching file types are shown, and exports use a new name if that name already exists.

[Install](INSTALLER.md) · [Updates](UPDATES.md) · [Credits](../CREDITS.md)
