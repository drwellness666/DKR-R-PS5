# Architecture review — 18 October 2026

This follows the [pre-release review](PRE_RELEASE_REVIEW.md). It records which
changes are implemented and which platform transitions still need qualification.

## Decisions applied

| Area | Decision and implementation |
|---|---|
| TV settings | Store resolution, refresh rate and HDR in one versioned `preferences/tv-output.txt` record. Apply and Restore each use one atomic replacement. Valid legacy records are imported on the first save and remain untouched. A malformed current record never falls back to stale legacy settings. |
| Rendering | Retain the lock-free HDR calibration snapshot. Preference parsing, migration and synchronization remain outside the present worker. |
| Update storage | Estimate the download, new program files, previous program files, largest rollback replacement and a 16-MiB allowance. Show the estimate in the existing update card; check again before downloading. A measured shortage stops before any package transfer. Missing filesystem telemetry is explicitly unknown. |
| Update startup/recovery | Reject non-regular automatic-check preferences and recovery markers without blocking. Unexpected entries remain preserved. |
| Installer storage | Use the same overflow-safe available-space calculation and report required/available space in the installer log. Keep write verification and recovery when telemetry is unavailable. |
| Recovery retention | The native updater removes only its verified completed/abandoned transaction. Manual installer history and unrecognized entries remain preserved. No automatic save, texture, log-history or recovery-version deletion is introduced. |
| Interface and installation | Keep the existing Touchpad menu, one universal installer, both owned US ROMs and native in-game updates. No extra launcher, PC runtime service or receiver is added to normal play. |

The storage estimate is a preflight, not a reservation. Other programs can consume
space afterward; existing transaction recovery still handles write failures.
Both staging and recovery paths are on the installation's internal storage.

## First public installation

**Version 1.0 starts with a fresh installation.** Nothing has been publicly
released, so private development previews do not require an upgrade or metadata
migration. The complete installer, ROM import, generated controller sounds,
registration and first launch are the final installation test path.

The 1.0 app declares its HDR capability from the beginning. Future compatible
updates keep that declaration unchanged. Keep the existing refusal when the
same title ID belongs to different registered metadata or modified files;
first-release scope is not a reason to overwrite another installation.

No preview migration work is a release gate. Direct `app.db` edits, broad cache
replacement and private registration sockets are not part of the installation.
The reviewed ShadowMountPlus registration behavior remains relevant only if a
future release intentionally changes title metadata.

Sources: [registration implementation](https://github.com/drakmor/ShadowMountPlus/blob/1.7/src/sm_install.c),
[public API](https://github.com/drakmor/ShadowMountPlus/blob/1.7/docs/api.md).

## Update compatibility

Freeze the reviewed v2 data-package contract for 1.0: 37 public program paths,
retained owner-generated audio, unchanged registered metadata and runtime ABI.
Changes outside that contract need a separately reviewed installer/protocol
transition. Never sign an incompatible package as a routine native update or
relax the old client's allowlist through untrusted manifest fields. A future
protocol should declare its minimum compatible client before downloading.

## Recompiled-project comparison

The [comparison](RECOMP_COMPARISON.md) records pinned Zelda64Recomp, BanjoRecomp
and Snap64Recomp sources. Audio scratch reservation is applied before playback;
saved renderer settings already precede setup. Shader-list prewarming needs a
separate DKR/renderer integration and is not silently enabled.

## Lifecycle and performance

Do not infer foreground/suspend state from the dummy SDL window. A native event
adapter needs verified event layouts, ordering and behavior on the target loader.
Until then, retain the current game-context gates and joined shutdown workers;
do not add a polling background payload or speculative event ABI. Controller
reconnection, PS-menu behavior and suspend/resume remain console checks.

Keep shader compilation, Tracks-preview water, audio underruns and frame pacing
separate in measurements. No speculative rendering workaround or global timing
change is included here. Shader-cache persistence remains opt-in. GPU/TV timing
and physical controller effects still need the final combined-build test.

## Verification

The actual preference adapters cover legacy import and preservation, current-record
authority, individual updates retaining the other fields, eight write/sync/rename
failure or process-cut points and successful retries. The renderer-snapshot test
still runs while synchronization is blocked. These process cuts test application
failure handling; they do not emulate physical storage power-loss behavior.

Storage tests cover the exact capacity boundary, a shortage before download,
unavailable/invalid telemetry, multiplication overflow and block-size fallback.
The real native transaction includes enough-space, low-space and unavailable-space
scenarios alongside its interruption and recovery tests. See [test status](VALIDATION.md).
