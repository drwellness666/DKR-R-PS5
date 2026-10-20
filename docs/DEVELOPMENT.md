# Developer guide

Players can use the [installer ELF](INSTALLER.md) without compiling anything. This page is for building, debugging or contributing.

| Topic | Guide |
|---|---|
| Full source build | [Build commands](BUILD.md) · [Windows/WSL2](WINDOWS.md) |
| Platform layout | [Architecture](ARCHITECTURE.md) |
| Renderer builds | [PS5 graphics](GPU.md) · [RT64](RT64.md) |
| Distribution | [Licences](LICENSES.md) · [Source inventory](../THIRD_PARTY.md) |

## Source and checks

`GAME_SOURCES.json`, `RT64_SOURCES.json`, `VULKAN_SOURCES.json` and `TITLE_ART_SOURCES.json` pin inputs. DKR-R 1.0.8 retains the checked 1.0.7 release-source layers and adds the upstream Windows startup-memory hotfix; the PS5 build reconstructs those layers before applying platform changes and generates both private camera draw kernels locally. Keep PS5 changes in adapters and reproducible patches; regenerate CPU code through the build pipeline.

[GitHub Actions](../.github/workflows/source-check.yml) defines the automated source checks. They cover installation/update recovery, storage, controls, menus, output and provenance. Hardware behavior still requires PS5 testing; [feature status](KNOWN_ISSUES.md#feature-status) records its scope.

`tools/export_public_source.py` exports an explicit allowlist. `tools/check_public_source.py` scans that export and its bundled archives for excluded data; `SHA256SUMS` records the files. Preserve matching source, build recipes and original notices when distributing binaries. Personal game data and working notes are excluded.

## Debugging

Keep the exact app, `$DKR_GAME_WORK/private/CANDIDATE.json` and `$DKR_GAME_WORK/native-link/llvm-pie.elf` together. For a manual build, record the receipt with:

```bash
python3 tools/record_native_build.py "$DKR_GAME_WORK"
```

Local game journals are under `/data/dkrr/diagnostics`. For optional PC capture, use an existing klog service or `tools/build_session_relay.sh SDK_ROOT`; do not run competing klog readers. The relay uses TCP 3232.

```bash
python3 tools/collect_session.py --capture --host YOUR_PS5_IP \
  --output /ABSOLUTE/private/NEW_SESSION --build-id BUILD_ID --seconds 900
python3 tools/analyze_session.py /ABSOLUTE/private/run.log \
  /ABSOLUTE/private/CANDIDATE.json /ABSOLUTE/private/symbols.elf
```

Analysis checks matching symbols before attributing addresses. `report_diagnostic_session.py` combines saved evidence locally. If a file manager cannot read a protected journal, `build_journal_reader.py` creates a reader restricted to that journal and build; see its usage. Do not change save/log permissions broadly.

## Publishing updates

Increase `installer_sequence` in `PORT_RELEASE.json` for each new program build. Build a fresh package with matching source/notices and test installation/recovery. Sign its channel with `tools/sign_update_channel.py` using the private release key outside the repository. Publishing source alone does not make an in-game update available.

The native updater requires compatible metadata, runtime and package paths. Keep the registered title name/version stable across normal updates; PORT_RELEASE.json and the signed update channel carry each actual release tag. See the [architecture](ARCHITECTURE.md). Keep credentials, signing keys, ROMs, generated code, extracted audio and private builds out of commits.
