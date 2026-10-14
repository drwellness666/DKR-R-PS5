# One integrated PS5 test build

Reviewed 2026-10-14. This is the current candidate track; previous private
artifacts are historical emergency copies, not a sequence of required tests.
The released source beta and private main branch are preserved. Hardware
acceptance of this combined candidate is still pending.

## What is in the same application

| Requested area | Integrated behavior | Physical acceptance still needed |
|---|---|---|
| DKR-R 1.0.6 | Pinned c3eda4d; both owned US payloads and matching language hooks | Start/play each revision |
| US 1.0 / US 1.1 | Play → Game version, takes effect next launch; established separate profiles | Choice persists and correct revision boots |
| English / French / German | Original Options → Language cycles existing own-ROM text in each US revision | Text/umlauts and save/reload |
| Menus/save names | OFL font loader, filled Play page, one-column settings, repaired sidebar marker, gamepad navigation | TV appearance and controller feel |
| Icon/background/start | Original upstream icon, identical paired pic0/pic1, hide splash after first completed presentation | Home-menu background remains an open shell/registration issue |
| Controller | Optional per-player R2/L2 driving layout, classic preset/undo; existing ordinary Rumble Pak/Memory Pak | Vibration stop, settings and Pak save/reload |
| Native display | 1080p/1440p/2160p and 60/120 Hz in existing Graphics → Game display; next-launch apply/reset | HDMI/TV result, buffer allocation, focus and repeated start |
| Frame delivery | Existing FPS overlay plus optional bounded present-return capture | Cold/warm same-track measurements of microstutters |
| Sound | Neutral mixer fast path retains bit-exact int16 output | Sustained audio under load and output changes |
| Textures | Existing private upstream-imported user pack remains separate | Visual result and performance comparison |
| Updates | ROM-free source release check/download, explicit own-ROM rebuild, exact predecessor incremental installation | Installation/repeat receipt; not an in-game automatic binary updater |
| VRR / ALLM | Existing graphics page explains PS5/TV settings; qualified high-refresh declaration in candidate metadata | Actual TV state; no app-controlled toggle claim |
| Native HDR | Current native WSI only advertises SDR | Backend work remains; no nonfunctional checkbox |
| Advanced DualSense | Adaptive resistance, detailed waveform haptics, native gyro bridge, speaker remain documented designs | Qualified public structures/units/reset and physical proof needed first |

The short water flash at Tracks-preview entry and reported microstutters are
open defects. No local test establishes a fix. Adventure intro depends on
original progress/start conditions and also needs comparison with a fresh slot;
existing saves are never reset merely to make an intro appear.

## Build and installation

Follow [GAME_BUILD.md](GAME_BUILD.md) for the pinned GPU/SDL/RT64 toolchain and
owned ROM inputs. With both explicit `DKR_ROM_V77` and `DKR_ROM_V80` set, run
`bash build_integrated.sh` instead of the final `build_game.sh` step. This builds
one app with all the supported optional controls/output/diagnostics above.
No extra ROM or texture data is distributed. A single-ROM source build remains
available through the documented `build_game.sh` route.

Absent a saved preference, output stays 1080p/60 and new controller effects are
not enabled. Existing preferences and bindings are honored; inspect them before
comparing results. Selecting 120 Hz does not claim 120 FPS or VRR operation.

The combined updater allows a metadata change only with the explicit
`--allow-high-refresh-metadata` option, and only `attribute3: 0 -> 0x40040` with
all other parsed fields and own title identity unchanged. Exact old/new bytes
are preflighted for all three own param.json copies; icon, background and paired
launch art are still guarded. Foreign, symlinked or collision files stop the
entire preflight before writes. Files are replaced atomically one at a time;
this is not a bundle-wide power-loss transaction. No app.db editing or private
system IPC is used. The shell's cached registration is not refreshed by copying
metadata, and may still require the loader's normal own-title registration.

## One test session

1. Start the usual loader, close only DKR-R normally, verify the prepared
   update manifest offline, then install the one integrated app once.
2. Start at 1080p/60 with the existing gameplay settings. Confirm game picture,
   sound, menu text, splash disappearance and correct build ID first.
3. Test US 1.0 and US 1.1 through the same menu and normal title restart; verify
   language selection, save/load, optional driving preset and ordinary vibration.
4. Run the same Tracks-preview entry and race cold/warm with the optional
   60-second measurement. Record texture/cache/graphics settings for comparison.
5. Change one output option at a time, restart the same app, and record its
   active buffer mode plus the LG C1's actual HDMI information. Test 4K/60 before
   120 Hz; use Restore 1080p/60 if an optional mode fails.
6. Record icon/home-menu background separately from launch splash, focus return,
   normal exit and a second launch.

A loader refusal with no fresh game session is investigated as a loader/start
issue first. An application fault uses this build's journal/ELF. A rendering or
timing defect is narrowed by optional settings in this same build. The old
working archive is retained for a deliberately requested emergency recovery,
not automatically substituted after every failure.

Diagnostics and their limits: [DIAGNOSTICS.md](DIAGNOSTICS.md).
