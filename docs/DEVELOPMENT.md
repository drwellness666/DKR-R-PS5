# Development

DKR-R 1.0.6 and all build inputs are pinned in `GAME_SOURCES.json`,
`RT64_SOURCES.json`, `VULKAN_SOURCES.json` and `TITLE_ART_SOURCES.json`.
Keep platform changes in `src/` and reproducible patches. Regenerate selected
CPU payloads through the normal pipeline; do not hand-edit generated functions.

| Topic | Document |
|---|---|
| Build / Windows | [Build](BUILD.md), [Windows](WINDOWS.md) |
| Native installer target | [Installer](INSTALLER.md) |
| Saves and startup | [Saves](SAVES.md) |
| Renderer | [Graphics](GPU.md), [RT64](RT64.md) |
| Debugging / updates | [Diagnostics](DIAGNOSTICS.md), [Updates](UPDATES.md) |
| Distribution / tests | [Licences](LICENSES.md), [Validation](VALIDATION.md) |

The local build assistant is `setup.py` and `tools/setup_ui.html`. It binds to
loopback, uses a per-run token, validates input ROMs and invokes the manual build
recipes. It keeps build attempts separate and checks installer completion markers.
It is not a runtime service required on the PC while playing.

The source gate is `.github/workflows/source-check.yml`. Host tests cover adapters,
source preparation, menu geometry, diagnostics and update refusal/recovery cases.
They do not replace native display, controller or cold-boot save tests.

The export is a positive allowlist from `tools/export_public_source.py`.
`check_public_source.py` scans files and bundled archive members; `SHA256SUMS`
records the complete export. Keep ROMs, extracted game assets, generated CPU
payloads, game binaries, saves and raw console logs out of commits and issues.
Imported licences and copyright notices must be retained.
