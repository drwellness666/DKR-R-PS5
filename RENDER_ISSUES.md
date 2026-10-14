# Rendering investigations for the PS5 preview

These are reported defects, not confirmed fixes. Latest report: 13 October 2026.

| Case | User observation | Current local work / next discriminating test |
|---|---|---|
| Tracks menu | Menu graphics/text missing while music and navigation sounds remain | Latest user confirms preview appears with a brief entry-only water flash; complete Track-menu labels/navigation still need confirmation |
| Adventure intro | Intro skipped despite `skip_intro=0` in the observed profile | Compare a fresh test save to an existing save, record buttons held at the transition; don't replace the user's progress |
| Game save-slot names | Labels are now visible in the current v77 test (user confirmed) | Keep distinct from full persistent save/write/reload acceptance |
| Recompiled interface | Partial text missing, awkward diagonal field focus | Play/Online text visible in user photos; other pages and physical field navigation still need acceptance |
| Water at Tracks-preview entry | Flickers only at the beginning of preview/video, later settles | Capture scene entry; repeat same track cold/warm, Accurate/Modern, pack off/on, then one water A/B switch at a time |
| Microstutters | Occasional hitches while playing | Capture CPU/GPU/presentation timings for the same scene and settings; compare pack off/on and cold/warm; cause not assigned |

## Water-specific source findings

DKR-R 1.0.6 already contains dedicated water UV-copy/coalescing optimizations,
semantic interpolation identities, scroll-period handling and a title-demo tile
interpolation exception. The exception is for menu ID 0; it does not automatically
cover every Tracks preview. The current port disables the separate enhanced-water
option at build time. That does not disable ordinary retail water or these bridge
optimizations. These facts narrow tests but do not identify the reported fault.

Do not change all water quality, simulation speed, clipping or global interpolation
to mask an unmeasured transient. A proposed fix must preserve the stable later
frames, Adventure races, shadows and both supported US revisions.

## Explicit next-test diagnostics

The native entry can read the optional regular file
`/data/dkrr/render-diagnostics.txt` before the runtime starts. The whole file is
limited to 512 bytes. Exact keys below accept `0` or `1`; blank/comment lines are
allowed. An unknown/duplicate/malformed key rejects the whole file. Nothing is
written to saves or normal graphics settings. With the file absent, this facility
does nothing. Runtime flags are set only after complete validation.

For a timing/identity capture:

```ini
track_profile=1
water_profile=1
interpolation_trace=1
power_profile=1
```

`power_profile` enables the existing upstream timing scopes: shader/pipeline
construction, upload/fence waits, UI work and 300-interval presentation windows.
It does not add a new per-frame logger or change render scheduling. For settings
and launcher measurements, additionally use `launcher_profile=1`. Explicit zero
removes the environment flag; this matters because one upstream rate report
checks for flag presence. All nine keys still share atomic, bounded validation.

After copying only the game's own runtime log to the host, summarize it locally:

```bash
python3 tools/summarize_performance.py /absolute/path/to/runtime.log > performance-summary.json
```

The summary retains each reported percentile window and the latest cumulative
region totals. It does not average percentiles, add repeated cumulative counters,
infer physical scanout, or label wall-clock waits as CPU utilization. Missing
measurements remain unavailable. Unrelated raw log lines, paths and messages are
not copied into its output. Keep the original logs private.

Capture the same track/route for at least two 300-interval windows, once on first
entry and once after repeating the track. Label pack on/off, Accurate/Modern,
resolution and output target outside the log. Compare pipeline construction and
texture waits alongside presentation gaps. A reduced gap in one instrumented run
is not by itself a proven fix. The driver disk cache remains disabled by default. The explicit `shader_cache=1`
comparison uses the documented native exclusive database at an own writable
cache directory; missing/unknown directories refuse enablement.

Use these comparison keys **one at a time**, keeping all other settings identical:

```ini
water_uv_baseline=1
water_draw_baseline=1
water_scroll_baseline=1
```

Each restores a particular upstream baseline for comparison; this is a diagnosis,
not an improvement setting. Start a new process for each run because upstream
reads these switches once. Revert to the absent file or all-zero values after the
capture. Do not use timings from an instrumented run as an uninstrumented performance
claim. The parser is host-tested; console log capture and visual correlation remain
pending. No automatic upload or background PS5 connection is part of this facility.

## Current user acceptance, 13 October

The user confirms Adventure save-slot names are visible and the title icon now
appears. The Play pause page is sparse in the upstream design; the next native
build adds version selection and controller hints. The extra narrow native sidebar stripe is now removed entirely; the current
page uses its existing button colour. Real ImGui checks prove no extra marker
geometry/scissor is submitted and verify the actual Play picker/navigation. TV
appearance still needs checking.
The latest report reiterates microstutters during play and confines water
flickering to the beginning of Tracks previews. Adventure water stays stable.
The preceding runtime log has no upstream presentation percentile windows;
its aggregate RADV submission totals cannot locate individual hitches.
Timing capture is the next test, with baseline graphics and texture settings
preserved. No microstutter fix is claimed.

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
restart was requested; no new code or unsafe status payload is used for that
first comparison. This failure does not isolate the dual payload or UI as its
cause. Earlier visible picture/audio acceptance of the splash fix remains a
separate historical result.

## Original Adventure intro conditions

The pinned retail `fileselect_loop` starts `MENU_NEWGAME_CINEMATIC` only when
`settings->newGame` is true. A continuing save follows the regular hub route.
The Recompiled `skip_intro` option is a separate one-shot boot-logo/title
preference; it does not itself define this Adventure condition. Thus “skip_intro=0”
and a continuing save do not prove an Adventure cinematic defect. A fresh private
test slot, observed input at transition and game-state correlation are needed.
Existing progress is not reset or overwritten to force a cinematic.

Source: [retail menu](https://github.com/DavidSM64/Diddy-Kong-Racing/blob/06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6/src/menu.c).

No water or timing policy has been changed speculatively in this presentation
update. The entry-only water flash and reported microstutters remain separate
physical investigations, with the existing opt-in measurements retained.

## Prepared private-beta performance comparison

The default neutral-audio fast path has exact output parity against the actual
upstream mixer over all int16 values. Non-neutral gain, music and tones use the
original loop; queue priming and emulated AI timing remain unchanged. The host
benchmark proves less work in this path, not a cause or fix for the reported
PS5 microstutters.

For a separate shader-cache comparison, use `shader_cache=1` in the bounded own
diagnostics file. The pinned RADV build already includes the PS5 exclusive
database implementation. The adapter sets `MESA_SHADER_CACHE_DIR` to its own
`/data/dkrr/renderer/radv-shader-cache`, `MESA_DISK_CACHE_DATABASE=true` and a
64M size target, after ordinary-directory checks. It does not use read-only
`/app0`, delete old caches or fetch compiled shaders. Any refusal retains the
disabled baseline. Do not include cache files in a release or issue.

Compare the same original-texture race twice without caching, then cold/warm
with caching. Keep resolution/frame target/workers unchanged. Use the existing
upstream power profiler for at least two 300-interval windows and keep raw logs
private. Report presentation-return intervals separately from physical scanout
and audible glitches. A cold cache can add disk work, so qualify both entry and
repeat/relaunch behavior before adopting this setting as a default.

Primary implementation evidence: the pinned public RADV platform history,
[cache on PS5 and exclusive mode](https://github.com/mihawk-99/PS5_Vulkan/blob/main/docs/RADV_PHASE.md),
and [Mesa environment variables](https://docs.mesa3d.org/envvars.html).
These are other projects' evidence, not a test of DKR-R on firmware 13.60.
