# Development

The runtime currently targets DKR-R 1.0.6 at the revision in `GAME_SOURCES.json`. Public dependencies are pinned by revision and SHA-256. Keep platform changes in `src/` and reviewed patches; never edit generated game functions to conceal a bug.

- [Manual build](BUILD.md), [GPU components](GPU.md), [RT64](RT64.md).
- [Native controllers](CONTROLS.md), [display](DISPLAY.md), [diagnostics](../docs/DIAGNOSTICS.md).
- [Updates](UPDATES.md), [licences](LICENSES.md), [validation scope](VALIDATION.md).

The local assistant is `setup.py` with `tools/setup_ui.html`. It binds only to loopback, uses a per-run token, checks ROM contents and calls the same build/install tools as the manual route. It never invokes a cloud build. It keeps game build attempts separate and requires authoritative install markers before sending the next installer.

ROM-free checks run through `.github/workflows/source-check.yml`. The assistant tests cover HTTP access, upload bounds, the actual ROM rejection path, build ordering, work-directory isolation and installation success gates. Network fixtures use only loopback. Native output and controller behavior require console testing in addition to host tests.

Do not commit ROMs, extracted assets, generated CPU payloads, game binaries/installers, saves/Paks or raw console logs. Preserve original licences and copyrights. Include a reproduction and targeted validation with contributions.
