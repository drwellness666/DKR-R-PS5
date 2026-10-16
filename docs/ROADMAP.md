# Roadmap

## Installation and startup

- One ROM-free installer ELF, ROM selection/import on the PS5 and normal icon launch.
- Direct native save storage; remove the metadata helper rather than automate it.
- Cold-boot/save tests with the PC disconnected.

[Installer design and current limits](INSTALLER.md). The existing source-build
route remains available; these installer changes are not yet released features.

## Controller features

- **Optional gyro steering:** calibration, recenter, sensitivity and stick fallback.
- **Optional adaptive L2 shot feedback:** resistance and impulse on actual item use;
  reset on release, pause, disconnect and when disabled. Keep Classic gas tapping.

## Stability

- Isolate the brief Tracks-preview water flash and reproducible frame-time spikes.
- Test more displays/loaders, texture packs, online/mod use and Controller Paks.
- Simplify compatible updates after the standalone installation flow is proven.

No dates are promised. [Known issues](KNOWN_ISSUES.md) · [Validation](VALIDATION.md)
