# Repository contents

## Repository and source download

| Contents | Purpose |
|---|---|
| English README + German translation, `docs/`, changelog | Installation, controls, problems, credits and help |
| `src/`, `installer/`, `patches/` | PS5 game integration and universal installer/update source |
| `tools/`, build scripts, `setup.py`, Windows/Linux starters | Rebuild the port and package an installer |
| `tests/`, `.github/` | Automated checks, bug reports and contribution templates |
| Pinned source manifests, `vendor/`, `licenses/`, `LICENSE`, `THIRD_PARTY.md` | Exact dependency sources, build inputs and required notices |
| `homebrew/`, artwork source manifest | App metadata, artwork preparation and attribution |
| `updates/native-channel.txt`, `updates/channel.txt` | Signed native version information; archived private-preview channel |
| `SHA256SUMS` | Checksums of the source files |

Technical sources and notices are needed to maintain and rebuild the port. The
main README and help index are the starting points for players.

## Separate release downloads

- **DKRR-Installer.elf** — imports the owner's ROMs from USB and installs the app.
- **DKRR-Update.elf** — manual update using the console's own ROMs.
- **DKRR-Update.dkru** — data-only package downloaded and installed by the game.
- **DKR-R-PS5-source.zip** — matching integration source and pinned build recipes.
- **SHA256SUMS.txt / BUILD-INFO.json** — download checksums and version/test information.

The universal ELFs contain compiled program code and upstream interface/title
artwork. They contain no complete ROMs or extracted controller audio. Version 1.0 is in preparation; [licences and artwork terms](LICENSES.md) still apply.

## Personal and build data

Owned ROMs, extracted PCM, private ROM-containing installers, full private app
bundles, ROM-derived debug symbols/generated CPU trees, saves/Paks, third-party
texture packs, raw console logs, addresses, credentials, update-signing keys and working backups are
not uploaded. Developer working notes and local test/build directories are also
excluded. Updates do not copy these private files into a release.
