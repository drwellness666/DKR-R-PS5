# Changelog

## 1.0 — in preparation

- Bind copied licences, title-art provenance, staged assets and the actual native runtime/tool to the build fingerprint.
- Verify exact component-notice bytes and reject stale application copies before packaging; include the platform/source notices and maintainer-confirmed font/artwork scope.

- Keep update-preference errors visible until a successful retry; explain a disabled install button when storage is insufficient.
- Open controller sounds through bounded, nonblocking descriptor reads; reject special files and incomplete clips without holding up worker shutdown.
- Ignore stale controller-speaker load/open/output results after cancellation, so toggling the feature back on can play a new cue.
- Read PS5 master gain once per audio block, including the equalizer/custom-music path; preserve the original mixer output at stable settings.

- Reserve audio conversion scratch buffers before playback; accepted blocks no longer grow those vectors during delivery.
- Keep optional diagnostic-file reads nonblocking at startup.

- Save TV resolution, refresh rate and HDR together; import valid older settings on first save without changing their files.
- Show estimated update space, including recovery copies; stop before downloading when measured space is insufficient.
- Reject pipe-shaped update preferences and recovery markers without blocking startup or removing unexpected entries.
- Recover preference changes after interrupted display, HDR or version-selection writes; synchronize the directory and reject named pipes without blocking.
- Continue ROM discovery on later USB drives when an earlier root is crowded; reject unrelated same-size files from their headers before bulk reads.
- Establish fresh installation as the first public 1.0 baseline; exclude private-preview migration and the old network-updater ELF from release requirements.
- Keep DKR-R’s original typography unchanged: Racing Banana menu faces, Jumpman headings and digits, Selawik reading text and the original fallback policy.
- Add optional HDR10 under Graphics → Display, with paper white, peak brightness, highlight expansion and a calibration pattern. SDR remains the default.
- Keep interface brightness separate from scene highlights and compose both into a 10-bit PQ output.
- Fall back to SDR when HDR buffers or HDMI output are unavailable; retain the existing 60/120-Hz and VRR paths.
- Validate the embedded shaders by actual off-screen Vulkan pixel readback; add native-format registration and font-atlas regression checks.
- Resume app registration after interrupted or obsolete temporary snapshots without changing existing temporary files or other app entries.
- Include original component notices with the source and installed app, including Jumpman’s embedding licence.
- Reuse one timestamp for optional controller feedback; skip that clock read when all feedback is off.
- Reject an unallocated HDR descriptor set before GPU commands are recorded.
- Publish coherent HDR brightness snapshots without making the present worker wait for preference reads, writes or filesystem synchronization.

Version 1.0 is not released yet. Final fresh-install/PS5 qualification and the
matching source/package review remain open. Tracks-preview water flicker and
occasional microstutters are not claimed fixed.

## Private development history

The entries below describe unpublished preview milestones, not public releases.
They are not supported migration baselines for 1.0.

### Preview 3.4

- Install Internet updates inside DKR-R, without a running ELF receiver or separately started helper.
- Download a signed data-only package; no downloaded installer code is executed.
- Prepare app-specific directory access during installation; leave system and other app directories unchanged.
- Save and join game workers before replacing program files, keeping the installed app folder in place.
- Restore interrupted native transactions at startup. Keep old copies intact until recovery completes; avoid rewriting already restored files when filesystem access is denied.
- Retain the manual ELF route for offline updates.
- Keep unchanged runtime/metadata files in place; synchronize journal ancestry, retire completed transactions before cleanup and fail closed on metadata read errors.
- Add sanitizer tests for every replacement cut, repeated recovery, interrupted cleanup, partial markers, invalid packages and filesystem failures.

### Preview 3.3

- Signed Internet-update check and download under About DKR-R. Optional launch checks are off by default.
- Native ready/commit handoff to the console's existing ELF receiver; installation waits for the game to finish all file access.
- Exact size/SHA-256 verification, restricted HTTPS redirects, cancellation and separate manual update fallback.
- Accept native PS5 zero-link-count metadata for read-only update preferences and atomic preference replacement.
- No extra crypto dependency: the port reuses its pinned Monocypher and Mbed TLS sources.
- Avoid a disabled performance report's clock read and the duplicate clock read during active generic captures. Periodic rollback diagnostics now require explicit profiling.
- Tracks-preview water, reported microstutters and HDR remain open.

### Preview 3.2 — controller experience

- Show current per-player bindings on an original vector controller diagram.
- Replace the long mapping list with an action selector; keep alternate bindings and restore/copy actions under More binding options.
- Compact PS5 options and move gyro fine-tuning behind a disclosure. Normal play no longer shows speaker diagnostic instructions.
- Add original horn, weapon-balloon pickup/upgrade, silver-coin, gold-balloon and boost-pad cues to the existing controller-sound switch.
- Observe real audio calls in both US revisions; suppress nonlocal, paused, loading and rollback events. Item use has priority and cues never accumulate.
- Prepare 32 short cues per ROM locally, including sample-bank rendering of all eight silver-coin jingles; no PCM is embedded in shared ELFs.
- Show which ROM’s controller sounds are being prepared during installation.
- Extend the automated checks for real game-call routing, live mapping geometry and malformed compressed sequences.


### Preview 3.1 — console menu refinements

- Put Controls, Graphics, Sound and Textures next to Play; preserve existing settings and page identities.
- Replace overflowing horizontal subsection tabs with one full-width section selector, using Cross to open/select and Circle to cancel.
- Separate Graphics into Display and HUD & Detail; simplify the paused-race page.
- Reuse loaded controller-sound cues and the prepared missile mix instead of reading/mixing them again on every use.
- Store identical title images only once inside each installer/update ELF; installed artwork is unchanged.
- Recommend the optional DKR REMASTERED pack by sr.gu, with its official community link.

This candidate does not claim a fix for Tracks-preview water flicker or measured
FPS gains. Earlier private preview artifacts remain archived locally.

### Preview 3 — universal installation and native PS5 extras

- One ROM-free installer ELF: import both owned US ROMs from USB on PS5.
- One update ELF: retain ROMs, settings, progress, Paks and texture packs; no previous PC build needed.
- Verified staging, release-version checks, retained recovery copies and interrupted-update recovery.
- Direct native saves; remove the old preview's separate save-metadata helper.
- Controls → PS5: optional balloon light colors, gyro steering, adaptive L2 item feedback and controller sounds for all five balloon types.
- Generate controller audio locally from the owner's ROMs; shared ELFs exclude those samples.
- Match display as the default Modern rendering target, following native 60/120-Hz output.
- English main README with German translation, shorter player guides and a clear PS5/upstream feature comparison.

The new install/save/controller paths are experimental. Native builds and 53
host check commands pass; their full console verification remains open.

### Preview 2.1

Shorter documentation, Windows/WSL2 instructions and clarification of the old
save-support startup dependency. Gameplay code unchanged from Beta 2.

### Preview 2

Dual-US selection, shared settings/textures with separate progress, original
EN/FR/DE game text, classic controls, balloon lights, 4K menu scaling, experimental
120-Hz/VRR output, PS5 title artwork and local diagnostics.

Known issues remain: brief Tracks-preview water flicker and occasional
microstutters. [Details](docs/KNOWN_ISSUES.md).
