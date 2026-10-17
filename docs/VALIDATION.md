# Test status

## Automated checks

The automated checks include **20 installer/update cases** with injected write, synchronization and activation
failures, stale registration snapshots, occupied temporary slots, crowded earlier
USB drives and header-only rejection of unrelated same-size files. Tests also cover shared settings, both language bridges, menu layout,
controller adapters, direct save handling and diagnostics. Menu tests exercise
the real ImGui section selector at narrow and wide widths, D-pad traversal,
Cross selection and Circle cancellation. Speaker tests check cue reuse and
revision invalidation; payload tests verify duplicate-file aliases. The original
**18** cue outputs remain byte-identical. **64** cues are prepared from the
owner’s two ROMs. New tests exercise actual US77/US80 sound wrappers, native
player/context gating and compact binding UI. The bounded CSeq decoder also
passes 8,000 malformed-input cases under address/undefined-behavior sanitizers.

Signed-update tests exercise the actual native worker with 12 service scenarios,
all manifest truncations and byte mutations, 10,000 malformed inputs, wrong
signatures/digests, redirects, cancellation and failed native preparation. No ELF socket API is supplied in the worker test, so a receiver dependency would fail to link. The legacy network
installer is tested before commit, across partial commit messages and while the
game retains its lease. Update-preference tests also reproduce the native
zero-link-count metadata and check repeated save/load without changing an
existing inode.

The in-process installer is exercised in **223 subprocess operations** under
address/undefined-behavior sanitizers: cuts after every public-file step and the
manifest, repeated recovery, write/rename failures and malformed signed tables.
Cases also cover persistent write denial, partial activation markers, interrupted
cleanup with reordered directory entries, metadata errors, journal-directory
synchronization and upgrading the actual game lease. Owner data and the app's
directory identity are retained; an old executable mapping remains valid during
replacement.

The native PS5 game and all three universal payloads compile and link with the public
homebrew SDK. Every one of the **37** embedded program files was read back from
each ELF and matched the app. ROMs and extracted controller PCM are excluded.
The exact source gate is [source-check.yml](../.github/workflows/source-check.yml).

All three native preference adapters also exercise interrupted legacy/numbered
writes, occupied slots, preserved symlinks, failed directory synchronization and
nonblocking rejection of named pipes.

The [pre-release review](PRE_RELEASE_REVIEW.md) records remaining release gates
and the scope of the source audit.

## Original typography and HDR

The original upstream font loader, all three font sources and typography metrics
are compared exactly with the pinned DKR-R archive. Its actual ImGui atlas test
passes with 26 font banks and 52 visible glyph checks. A blocked-`fsync` test also
confirms that HDR render snapshots keep working while a preference save waits;
concurrent calibration changes are read as complete, coherent choices.

## Confirmed on PS5

Earlier sessions confirmed gameplay picture/sound, both US revisions and
save/reload, shared settings/textures, readable menus at 4K, classic controls,
rumble, balloon light colors, icon/background and launch-image handoff.
4K/119.88-Hz output with VRR Automatic was confirmed on an LG C1, using firmware
13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite and ShadowMountPlus.

## Still to verify

- The new universal ELF's complete install/update and app registration on PS5.
- Native HTTPS reachability, the one-time app-directory permission grant and a complete receiver-free update/restart on PS5.
- New native save handling after a complete reboot, without the old preview's helper.
- Physical gyro, adaptive L2 and controller-speaker behavior.
- Development HDR title migration, HDR/SDR HDMI output, brightness/UI composition, HDR with VRR/120 Hz and GPU cost.
- More tracks, Paks, mods, online modes, displays and a complete Windows/WSL2 build.

These are final qualification items for version 1.0. Host checks do not prove
a feature's physical-console behavior or constant 120 FPS.

## Why SDK tests do not replace a console

The bundled SDK is a cross-compiler and PS5 API environment; it includes no PS5
hardware emulator. Its sample `make test` sends an ELF to `PS5_HOST` through the
ELF receiver. It is a console test, not a PC simulation. This project therefore
runs its own host tests and native build checks without requiring a console.

Sources: [SDK usage](https://github.com/ps5-payload-dev/sdk#usage),
[bundled-version sample test target](https://github.com/ps5-payload-dev/sdk/blob/f5d40f02770e9b0001dd9e18056eca70b3da8b40/samples/hello_world/Makefile).
