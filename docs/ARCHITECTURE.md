# Platform architecture

This page describes the PS5 integration for contributors. Players can start with [installation](INSTALLER.md).

## Storage

| Content | Location |
|---|---|
| Replaceable native app | `/data/homebrew/PPSA94678` |
| Owner-supplied ROMs | `/data/dkrr/roms` |
| Shared settings/textures and separate revision saves | `/data/dkrr/profile-us-v77`, `/data/dkrr/profile-us-v80` |
| Installation records and recovery | `/data/dkrr/installation` |

The universal installer imports both clean US ROMs from USB, prepares controller sounds locally and registers its source with ShadowMountPlus. It preserves unrelated registrations and never edits `app.db`. Both ELF variants contain program code without ROMs or extracted controller PCM.

Staging and recovery live outside homebrew scan paths. Installed manifests, file checks, a game lease and durable transaction records prevent replacement of a cooperating running app and support retries after interruption. Unknown or modified installations are refused. Installer recovery versions are retained; no automatic save or recovery-history deletion is performed.

## Native updates

The game uses system HTTPS to check a bounded Ed25519-signed release manifest. Title, sequence, package size and SHA-256 are checked; download hosts are restricted to the project's GitHub release route. Certificates and host verification remain enabled. No token or private signing key is embedded.

The client stages a data-only package, saves and joins its workers, takes the exclusive installation lease, then replaces changed files with the executable last. Metadata and the loaded runtime stay in place. A synchronized journal and verified old copies allow startup recovery. The installed folder keeps its identity. The installer provisions write access only to this app's verified entries; no parent homebrew/system folder is made writable.

The v2 update contract covers 37 public paths, unchanged registered metadata/runtime ABI and retained owner-generated sounds. Changes outside that contract require a separately reviewed manual upgrade. Downgrades, mismatches, unexpected links and unknown files are refused. The game updater never contacts an ELF receiver or executes a downloaded installer.

## Rendering and controls

The native title links RT64, Plume and PS5 Mesa/RADV. Output resolution/rate and internal rendering are separate from the original game simulation. HDR retains SDR scene/UI rendering and composes them into 10-bit BT.2020/PQ output; its extra layers are allocated only for HDR.

Controller cues observe original game audio calls for both revisions. Per-player mailboxes, context checks and priority rules suppress stale, nonlocal, paused or loading events. Bounded speaker caches are loaded on the worker. Optional effects start off and live in the existing Controls page.

[Build](BUILD.md) · [Checks and debugging](DEVELOPMENT.md) · [Feature status](KNOWN_ISSUES.md#feature-status)
