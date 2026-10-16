# Save data: current behavior and direct-storage target

Save files stay on the PS5 across normal restarts and power-offs. US 1.0 and
US 1.1 have separate progress; their settings and texture selection are shared.
The PC does not emulate the game, stream it or host the saves.

## Current limitation

The accepted test build uses a small **console-side** metadata service. DKR-R's
file checks need a hard-link count; the native title's access to the tested
`/data` files reports zero, while the background payload reports the real count.
The port checks file identity, size and modification time before using that value.
The service does not read/write save contents, merge revisions or recover saves.

It was already running in the accepted play/save sessions. Closing the game does
not stop it, but a console reboot does. Those sessions therefore do not establish
that a fresh boot without the service can load progress. Do not delete profiles
or overwrite existing progress to work around this limitation.

A separate service start after every boot is **not the intended finished flow**.
The current assistant button is a temporary fallback, not a PC runtime dependency.
Existing autoloaders can start console payloads, but automating this service alone
does not remove the underlying porting issue.

## Direct native storage

The next implementation should remove the metadata service, not merely hide it.
First verify the native file-access path and ABI, then adapt the save backend to
supported native file/container operations while preserving validation, exclusive
access, atomic replacement, backups and separate revision ownership.

The two pinned SDK `sys/stat.h` files are byte-identical, and the saved console
trace also reports zero from direct SCE stat and fstat. A simple header-version
mismatch is therefore not established. ShadowMountPlus provides `/data` through
a sandbox nullfs view; whether that view or a lower native call explains the
missing fields still needs a controlled native check.

Do not substitute a made-up link count or remove save protections. The public
native template recommends `/download0` for ordinary configuration but does not
guarantee its retention after title/cache removal; moving progress there alone
is not a complete save-storage solution. Native SaveData/container support needs
verified public bindings and a tested lifecycle before it can be called implemented.

## Existing storage

| Data | Location |
|---|---|
| Shared settings and primary texture library | `/data/dkrr/profile-us-v77` |
| US 1.0 progress, Paks and backups | Original folders under `profile-us-v77` |
| US 1.1 progress, Paks and backups | Original folders under `profile-us-v80` |
| Selected ROMs | `/data/dkrr/roms` |

The technical v77/v80 names mean US 1.0/1.1. Existing data remains in place during
this work. [Installer target](INSTALLER.md) · [Validation](VALIDATION.md).
