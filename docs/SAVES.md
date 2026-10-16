# Save data and startup support

Save files stay on the PS5 across normal restarts and power-offs. The save helper
is **not a save recovery/import program** and does not hold progress in RAM.
Do not delete a profile or create replacement progress if an existing save cannot
be loaded after changing the loader setup.

## Why the current port has a helper

In the tested native-title filesystem view, the PS5 returned incomplete file
metadata. DKR-R's save checks need the missing link-count information to reject
unsafe aliases. A small background payload supplies only that metadata for the
port's own save paths, over console-local loopback. It does not read save contents,
write saves or expose a service to the LAN.

The helper was active during the accepted save/reload tests. It stays available
across game close/relaunch but stops when the PS5 reboots. A working test session
therefore does not establish that a fresh boot without it can load the same saves.
This is a current startup dependency on affected loader/filesystem setups.

## Current workaround: start with the existing autoloader

WebKit Autoloader 0.6.1 supports an `autoload.txt` beside its payloads. Configure
the helper there **once**, after the existing jailbreak/mount payloads, so normal
future boots need no PC command or separate helper app launch:

1. Build `bin/dkrr-save-metadata.elf` with the normal build recipe.
2. Copy it into the **existing active** `ps5_autoloader` folder, alongside its
   current payloads. Internal storage is `/data/ps5_autoloader`; a USB config has
   priority. Use the same folder your console already loads.
3. Back up the existing `autoload.txt`, keep its current entries and append:

   ```text
   !1000
   dkrr-save-metadata.elf
   ```

4. On a later boot, launch your usual WebKit Autoloader, then DKR-R from its icon.
   Check an existing save after that boot before treating this setup as tested.

Do not create a replacement autoload chain or duplicate kstuff. When there is no
`autoload.txt`, the autoloader can delegate startup to Payload Manager, which has
its own autoload settings. Add the helper to that existing manager's startup list
instead of disabling it by creating an unrelated new config.

This workaround follows the [Autoloader 0.6.1 documentation](https://github.com/itsPLK/ps5-webkit-autoloader/blob/v0.6.1/README.md).
It has not yet been configured and cold-boot tested by this port's release checks.
The assistant's **Start save helper** remains a manual fallback for the current
session; it should not be a daily step once the existing autoloader is configured.

The intended finished installer must manage this dependency itself. A separate
manual startup step is not the target user experience. See [Installer](INSTALLER.md).

## Storage layout

| Data | Location |
|---|---|
| Shared settings and primary texture library | `/data/dkrr/profile-us-v77` |
| US 1.0 progress, Paks and backups | Original folders under `profile-us-v77` |
| US 1.1 progress, Paks and backups | Original folders under `profile-us-v80` |
| Selected ROM files | `/data/dkrr/roms` |

The technical `v77`/`v80` names are preserved for compatibility; they mean US 1.0
and US 1.1. Switching versions shares configuration, not Adventure progress.
Back up profiles before manual changes. Do not post saves or profiles publicly.
