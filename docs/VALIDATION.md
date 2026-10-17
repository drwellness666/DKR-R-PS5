# Test status

## Automated checks

The current implementation passed **57 host check commands**, including **15
installer/update cases** with injected write, synchronization and activation
failures. Tests also cover shared settings, both language bridges, menu layout,
controller adapters, direct save handling and diagnostics. Menu tests exercise
the real ImGui section selector at narrow and wide widths, D-pad traversal,
Cross selection and Circle cancellation. Speaker tests check cue reuse and
revision invalidation; payload tests verify duplicate-file aliases. The original
**18** cue outputs remain byte-identical. **64** cues are prepared from the
owner’s two ROMs. New tests exercise actual US77/US80 sound wrappers, native
player/context gating and compact binding UI. The bounded CSeq decoder also
passes 8,000 malformed-input cases under address/undefined-behavior sanitizers.

Signed-update tests exercise the actual native worker with 14 service scenarios,
all manifest truncations and byte mutations, 10,000 malformed inputs, wrong
signatures/digests, redirects, cancellation and missing receiver. The network
installer is tested before commit, across partial commit messages and while the
game retains its lease. Update-preference tests also reproduce the native
zero-link-count metadata and check repeated save/load without changing an
existing inode.

The native PS5 game and all three universal payloads compile and link with the public
homebrew SDK. Every one of the **37** embedded program files was read back from
each ELF and matched the app. ROMs and extracted controller PCM are excluded.
The exact source gate is [source-check.yml](../.github/workflows/source-check.yml).

## Confirmed on PS5

Earlier sessions confirmed gameplay picture/sound, both US revisions and
save/reload, shared settings/textures, readable menus at 4K, classic controls,
rumble, balloon light colors, icon/background and launch-image handoff.
4K/119.88-Hz output with VRR Automatic was confirmed on an LG C1, using firmware
13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite and ShadowMountPlus.

## Still to verify

- The new universal ELF's complete install/update and app registration on PS5.
- Native HTTPS reachability and the Internet updater's complete game-to-receiver handoff.
- New native save handling after a complete reboot, without the old preview's helper.
- Physical gyro, adaptive L2 and controller-speaker behavior.
- More tracks, Paks, mods, online modes, displays and a complete Windows/WSL2 build.

These do not prevent sharing the work as an experimental beta. Host checks do
not prove a feature's physical-console behavior or constant 120 FPS.

## Why SDK tests do not replace a console

The bundled SDK is a cross-compiler and PS5 API environment; it includes no PS5
hardware emulator. Its sample `make test` sends an ELF to `PS5_HOST` through the
ELF receiver. It is a console test, not a PC simulation. This project therefore
runs its own host tests and native build checks without requiring a console.

Sources: [SDK usage](https://github.com/ps5-payload-dev/sdk#usage),
[bundled-version sample test target](https://github.com/ps5-payload-dev/sdk/blob/f5d40f02770e9b0001dd9e18056eca70b3da8b40/samples/hello_world/Makefile).
