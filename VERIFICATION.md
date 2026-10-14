# Verification record

Updated 13 October 2026. Tests below have deliberately different scopes.

| Area | Evidence and result | Limits |
|---|---|---|
| Actual game on PS5 | One user played DKR-R 1.0.6 US v1.0 with picture, sound, DualSense and visible HUD | Track-menu/text/intro defects and occasional stutters reported |
| Existing-file startup | Native metadata adaptation matched backing inode, size, time and type; subsequent startup reached game presentation | Complete persistent save/load and newest dual/language build still need console acceptance |
| Single US v1.0 build | Full native compile/link and FSELF integrity PASS; v80 payload absent from symbol audit | Local build evidence |
| Single US Rev A build | Full native compile/link and FSELF integrity PASS; v77 payload absent from symbol audit | Local build evidence |
| Earlier prepared test titles | Isolated v77, separately compiled v80, and dual titles have 34 app files; current v77 installed and all files re-read with exact SHA256; first presentation, audio and existing-file metadata observed | Current v77 game/UI visible, save-slot names confirmed; persistent save/load and v80/dual gameplay pending |
| Fresh single-ROM recipe | Public source preparation, host recompiler/binutils, matching v77 decomp/ROM, CPU pipeline and full native title rebuilt in a new directory | SDL and GPU prerequisites reused from their previously verified builds |
| Pipeline equivalence | Selected-revision generation for v77 and v80 matches each previously qualified 40-file CPU output; other revision input/output absent | Does not prove every gameplay scene |
| Interface fonts | Real OFL console loader: 25 font banks; 75 visible ASCII/digit/German-glyph checks, populated 2048×2048 atlas; excluded font symbols absent from native binaries | Play/Online font rendering visible in user photos; other pages pending |
| Controller UI | Real ImGui navigation test passes field order, disabled items, scrolling, Cross activation and reverse movement | TV/physical-controller acceptance pending for the new layout |
| Save-editor navigation | Same real ImGui acceptance checks applied to the actual SavesGrid at wide panel size | TV acceptance pending |
| German selection | Actual generated US 1.0/1.1 Options loop/getter/setter: forward/reverse/neutral input, original text/save calls and unchanged settings/registers; real upstream EEPROM codec checksums and Adventure progress/records preserved | No German game-text rendering or full save/reload acceptance asserted |
| PS5 rumble | Actual SDL PS5 rumble function: amplitude conversion, stop, disconnect and ScePad errors pass with modeled transport | Physical DualSense vibration pending |
| ROM version selection | Actual adapter tested for single/dual builds, legacy and readable preferences, verified writes, missing ROMs, malformed/symlink/temp collisions, active revision and save preservation | Native dual build links with valid FSELF; controller choice/relaunch needs console acceptance |
| Native title artwork | Current candidate: 512×512 PNG icon and 3840×2160 single-level BC7_UNORM selection DDS, both byte-identical to preceding accepted artwork; launch artwork excluded; original inputs verified; only recipe/provenance exported | User confirms icon visible after registered metadata update; background still missing |
| Render diagnostics | Bounded parser rejects duplicate/unknown/malformed keys atomically and accepts exact opt-in flags | Water-flicker cause and console A/B result remain open |
| Stutter measurements | Eight-key diagnostics, actual threaded upstream profiler/report parser, separate percentile windows and latest cumulative totals pass locally | No PS5 timing capture or confirmed optimization outcome |
| Bundle updates | Actual C updater tested under ASan/UBSan with synthetic files: complete preflight, changed art/code, repeat/resume, wrong dependency, symlinks including parents and existing temp refusal | Per-file atomic update, not a bundle transaction; actual console predecessor must be checked |
| Save metadata | Real adapter with modeled native stat/transport passes containment, mismatched metadata, refusal, changed file, known counts and actual alias-count preservation | Model is not a hardware durability or adversarial-filesystem proof |
| Earlier upstream host suite | 121/121 tests passed with WebRTC disabled | No live matchmaking; focused checks rerun for relevant changes |
| Public export | Positive allowlist plus source/archive scan; generated game and ROM outputs excluded | Scan checks file properties, not a legal opinion |

## Host checks

Run the clean export scan before creating local build outputs:

```bash
python3 tools/check_public_source.py .
python3 tools/test_rom_checker.py
python3 tools/test_ps5_realpath.py
python3 tools/test_ps5_filesystem.py
python3 tools/test_ps5_getcwd.py
python3 tools/test_ps5_rdram.py
bash tools/test_save_metadata.sh "$PWD/work/save-metadata-tests"
python3 tools/test_upstream_title_art.py
python3 tools/test_performance_summary.py
python3 tools/test_revision_selection.py
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_game_language_bridge.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/test_native_game_bundle_update.py
```

After preparing the public game sources, exercise the actual UI components:

```bash
python3 tools/test_console_menu_navigation.py "$DKR_GAME_WORK"
python3 tools/test_console_fonts.py "$DKR_GAME_WORK"
python3 tools/test_game_language.py "$DKR_GAME_WORK"
python3 tools/test_controller_rumble.py "$DKR_SDL_WORK"
python3 tools/test_upstream_performance.py "$DKR_GAME_WORK"
python3 tools/validate_title_art.py "$DKR_GAME_WORK/private/title-art"
python3 tools/verify_game_patch.py "$DKR_GAME_WORK/downloads/DKR-R-1.0.6.tar.gz"
```

These use no ROM data or GPU. Fonts are compiled only from the pinned Selawik
source. `CHECKS.json` records current source/build status. Detailed private console
logs and user-derived outputs are intentionally excluded from the repository.

## Console session on 13 October

Before updating, an unqualified read-only application-status payload was followed
by a user-reported full power-off and unclean-shutdown warning. A kernel panic is
possible, not proved; the payload is quarantined and is not part of the source
export. No game update had been sent at that time. After the user restarted, the
known launcher, exact-predecessor updater, collision-refusing installer and
read-only metadata helper succeeded. All 34 own application files were checked
again by full SHA256. The current v77 game log records presentation and audio.
The launcher returned HTTP 503 despite actual startup; no repeated launch was
attempted. This is startup evidence, not a claim of stable gameplay or save/load.

The subsequent user test confirms Play/Online interface text and Adventure
save-slot names are now visible. Water still flashes briefly on Tracks-preview
entry, then stabilizes; the user reports no Adventure water problem. Occasional
microstutters remain. The original icon is visible after updating the own
registered metadata copies; the background is still absent. These confirmations
do not establish all-page/controller acceptance or durable save/load.

The historical dual-revision candidate (native SHA256
`9947dfc1361068591f021f4e19712c540c73e5a4a709adf39be168a26f8f0118`)
was installed with exact preceding-file guards. All 34 application files were
re-read with SHA256; the owned Rev A ROM was re-read with its full content SHA1.
Power/launcher profiling was enabled for that test. This executable was later
superseded by the splash correction; profiling was disabled again. The current
offline US-dual language candidate is described below and is not deployed.

## Native launch overlay correction

The next dual test was initially reported as black with audio gaps. The user
clarified that the PS5 **launch poster remained displayed**. Restoring the
previous byte-identical executable and disabling profiling left the same poster
on screen. The native game omitted `sceSystemServiceHideSplashScreen`, although
both accepted Plume/RT64 probes already use it. The correction calls that same
service once, after the first completed presentation; a US 1.0-only build is used
for the first acceptance test, with profiling disabled. This is distinct from
the requested home-menu selection background, whose visibility remains open.
The failing run is not valid evidence of a stutter cause or a broken dual payload.

The corrective US 1.0 native build returned `0x00000000` from the hide-splash
call. The user confirmed the game picture and normal audio were restored.
Profiling stayed disabled. This proves the splash handoff correction for that
build, not a microstutter optimization or acceptance of dual revision selection.
The home-menu selection background remains open; the launch poster is distinct.

## Later loader-state failure

After a closed-title update attempt, the corrected dual build was rejected with
“Can’t start game or app” before any fresh game log. The large full installer
transfer timed out while sending; no invocation was confirmed. A small
identity-guarded preference-directory installer succeeded, and all 34 game
files were verified. Restoring the exact previously accepted single build did
not recover native startup, including a manual start. Its full SHA256 is
`a878f66d9dd1d851d5bd974e8e8185f6c2c5b87e8af5a2c4c8df641bf55cfdad`.
The own mount link still points to the correct source. A normal console/loader
restart was proposed and declined during that session. The user subsequently
powered the console off; the work now continues offline; no new code or unsafe status payload is used for that
first comparison. This failure does not isolate the dual payload or UI as its
cause. Earlier visible picture/audio acceptance of the splash fix remains a
separate historical result.

## Original US language-cycle qualification

The authored composer checks the full reviewed allocated ELF images and exact
original instructions separately for US 1.0 and US 1.1. The private generated
code contains both new calls; no generated code is edited by hand. Run after
creating a matching private pipeline:

```bash
python3 tools/test_game_language.py "$DKR_GAME_WORK"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/test_recompiled_game_languages.py \
  --game-work "$DKR_GAME_WORK" --pipeline "$DKR_GAME_WORK/private/pipeline" \
  --elf-v77 "$DKR_GAME_WORK/private/v77/build/dkr.us.v77.elf" \
  --elf-v80 "$DKR_GAME_WORK/private/v80/build/dkr.us.v80.elf"
```

Both actual retail loops/getters/setters pass with address/undefined-behavior
sanitizers. The recompiler's known signed-LUI-shift idiom is excluded only from
shift-base instrumentation of generated translation units; authored fixtures
and adapters retain all checks. Leak instrumentation is disabled because the
host sandbox does not provide ptrace. Graphics, `load_menu_text` and EEPROM
transport use bounded fixtures; this is not rendering or hardware-persistence
acceptance. The original calls are observed, including their language argument.
Wrong-revision and conflicting-policy input are refused.

The preceding language-only candidate's native SHA256 was
`61d39b2dbde1287cae6aa7399012711e4fce4e0fca6dffdd02657baff1bd61f5`.
Full native compile/link, FSELF integrity, both revision payloads and the language
bridge symbol pass. Its 34 app files are frozen privately with individual SHA256
hashes. A fresh ROM-free source preparation applies all 19 reviewed patched
files and passes its complete saved-inventory recheck. The local equivalent
of the exported source CI, including the new ROM-free language bridge test,
also passes. This candidate has not been installed or tested on the PS5.

## Home-menu-only presentation and native sidebar cleanup

The preceding selection-only candidate was
`9718ba8fdb94ba4dfb05572a1097742cc2e36ed5d6d58c74e3e728012d06eb2e`.
It has 33 files and no pic1. Icon/pic0 bytes match the preceding build exactly.
Native compile/link and FSELF integrity pass. The current-page native stripe is
removed; actual ImGui draw counts prove no extra vertices/scissor are emitted.
Actual Play picker navigation passes choice order, next-launch ownership,
unavailable-ROM skip and return activation. Settings/save grid checks also pass.
The file-only poster-retirement and own-title read-only metadata-audit tests pass;
retirement and registered selection-art helpers compile natively. They have not
been sent to the PS5. Background visibility, no-poster launch and all physical
acceptance remain pending. See [TITLE_PRESENTATION.md](TITLE_PRESENTATION.md).

The new CPU language pipeline also runs through six actual Linux game boots:
US77/US80 × English/German/French, eight seconds each, regular exit status 0.
Each fresh 512-byte test EEPROM keeps its selected language and exact bytes.
This uses NullRenderer with no audible/visible or PS5 persistence assertion.
The host-only bridge object is linked from the same authored adapter; it is
private qualification output, not a public game download.

The private persistent logs additionally confirm 236 actual display lists
per language/revision run and a clean runtime stop; startup-only process output
is not used as guest-progress evidence. NullRenderer consumes these lists without
drawing them. No rendered glyph or audible-output result is inferred.

## Private combined update and exact rollback

The actual `src/update_game_preview.c` composes the existing bounded app updater
and known-poster retirement. Both complete preflights run before the first write;
ordinary temporary-path collisions are now checked in the app preflight as well.
The host test runs both directions with ASan/UBSan, repeated calls, partial resume,
unknown identities/content, temp collision and missing rollback backup refusal.
It verifies the complete synthetic baseline byte snapshot after rollback.

```bash
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tools/test_preview_update.py
python3 tools/test_preview_receipt.py
```

Both pass. The sender test uses an injected socket fixture, never a real console
connection. It checks tampering, split markers, wrong-operation markers, STOP,
bounded receipt size, transfer-only EOF and public-address refusal. Native update
and rollback payloads were compiled locally and their sizes/SHA256 verified.
No console execution, power-loss recovery or all-bundle transaction is claimed.
Both payloads contain private game code and are excluded from source exports.

## Private beta audio and paired presentation

The current presentation policy restores byte-identical pic0/pic1 from the
verified upstream sky image, following the latest user choice and template
layout. The accepted first-present hide-splash API remains unchanged. The app
has 34 files again; no retired-poster helper is used for this paired update.

`tools/test_neutral_audio.py ABSOLUTE_GAME_WORK` extracts the actual upstream
mixer loop and links its real equalizer. Sanitizer parity covers every int16
value and active gain/music/tone refusal. An optimized descriptive host benchmark
measured approximately 23.9 ms versus 0.31 ms per 100 blocks of 65536 samples.
Those artificially large blocks characterize this code path only; they are not
a PS5 frame-time or audible-stutter result. The fallback mixer and AI timing are
unchanged. Fresh pinned sources apply the complete 19-file patch and pass their
full inventory recheck. Native compile/link and integrity pass.

The owned-cache directory helper passes repeat, leaf/parent symlink, unknown-file
and traversal refusal tests. The nine-key diagnostics parser passes bounded
atomic rejection. `shader_cache=1` uses the existing documented RADV database
cache, at an own writable path, with a 64M target. It is opt-in, disabled by
default and unqualified on this PS5.

The source release updater passes fixture checks for beta/stable selection,
GitHub digest, tampering, traversal/private/git refusal and removal of tokens
on cross-origin redirects. It has made no authenticated network request in
these tests. A full future source-release rebuild/apply remains an E2E gate.

Current frozen paired/audio beta eboot SHA256: `11c2d3b4ae3eee8d4735cbaf6c4d1acd822ed37c5866efad7f0e77f8117dfcca`.
Its 34-file app manifest is frozen privately; the candidate is not deployed.

## Next optional controls candidate (offline, 2026-10-14)

- Actual patched runtime input with SDL virtual controller and snapshot backend:
  A/B/Z/L/R/Start, activation boundaries after upstream deadzone normalization,
  simultaneous accelerator/brake, Cross/Square alternatives, touchpad shortcut,
  neutral blocked/disconnected input, source uniqueness and four-player isolation pass.
- Actual settings binding writer records/loader and controls snapshot round-trip pass;
  no settings file schema or save directory change. This isolates the binding records,
  not the full native filesystem settings lifecycle.
- Actual preset and binding-row functions with real ImGui gamepad navigation pass:
  labels, vertical geometry, scrolling, Down/Up order, activation/capture and undo.
  Actual player selection and capture entry invalidate a preceding undo snapshot.
  Fixture presentation helpers replace fonts/device discovery; physical rendering is pending.
- Actual capture handler passes modeled controller ownership, touchpad cancellation
  without saving, bindable Circle and trigger capture boundary checks.
- Native target compilation, linking and FSELF integrity inspection pass. Only the
  candidate executable changes relative to the frozen private beta.1 app bundle.
- No console contact or installation in this offline work. Conventional trigger
  controls are not adaptive-trigger resistance, detailed haptics or native gyro support.

Reproduce the ROM-free controls tests after source preparation:
`python3 tools/test_controller_presets.py ABSOLUTE_GAME_WORK`.
Alternatively, omit the work path to fetch/hash-check only the pinned public DKR-R
and ImGui inputs. `--cache ABSOLUTE_DOWNLOAD_CACHE` can avoid those downloads.
SDL2 development headers/library and Clang are required. ASan/UBSan are enabled.

## Optional output candidate, offline 2026-10-14

Actual preference adapter, VideoOut gate and Vulkan display-surface code pass ASan/UBSan for ordinary and capable builds: all 1080p/1440p/4K and permitted 60/120 choices; reordered enumeration; missing modes and refused optional surfaces; device loss; missing functions/planes; malformed/oversized/NUL/duplicate inputs; symlinks, FIFO and temporary collisions; save preservation. Actual output UI fields pass a drawing-helper model for explicit Apply/Reset, pending versus active choice and ordinary-build 120-Hz refusal. Console visuals/navigation/timing/VRR/ALLM remain pending. Native UI/Plume compile, standalone probe compile without game references and complete fresh upstream patch/hash verification are included in the candidate evidence. Reproduce with `python3 tools/test_ps5_display.py [--cache ABSOLUTE_DOWNLOAD_CACHE]`.

## Integrated candidate, 2026-10-14

The current test track combines both US revisions/languages, controller/menu changes, optional output and diagnostics in one app. See [INTEGRATED_TEST.md](INTEGRATED_TEST.md). Local native integrity and host tests are separate from pending hardware acceptance. [DIAGNOSTICS.md](DIAGNOSTICS.md) records the exact private ELF attribution policy and optional PC capture. The combined updater permits only an explicitly requested attribute3 0 -> 0x40040 transition with all other fields preserved and all three own metadata copies preflighted. It does not refresh shell registration or modify app.db. Prior separate candidates remain historical archives, not a routine rollback sequence.

The integrated export passes all 27 workflow run checks locally (excluding only the apt package-install step on the already provisioned host). The same fresh pinned source passes the actual console font loader (25 banks / 75 glyph checks), upstream threaded performance trace and the 22-file zero-fuzz pristine patch/hash audit. The frozen private candidate has 34 app files, valid native FSELF integrity, both US payloads and the new diagnostics/output UI; exact native-ELF attribution also resolves the authored diagnostic source line from a synthetic matching record. No new PS5 test is claimed.

## Dependency preparation and documented full game build

The fresh game build first failed because git apply silently skipped dependency paths inside an enclosing repository; the previously passing 27-check suite did not establish clean-build readiness. The corrected helper isolates every patch, verifies changed bytes and preserves the outer Git metadata. Actual preparation applies 54 pinned patches, validates the runtime snapshot prerequisites and named compiler/runtime context header, and passes complete source-stamp reuse. The full native CMake DKRPortGame target and link/FSELF now pass with both owned US payloads using the existing qualified toolchain/CPU pipeline. The workflow adds nested-Git patch, complete real dependency preparation and exact output-preserving update-baseline tests. Native game behavior on the PS5 remains pending.
