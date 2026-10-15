# Changelog

## ps5-beta.2 — integrated source beta

- Both US revisions in one native app; original English/French/German choices.
- Share controls, graphics/audio, UI/HUD, mod settings and texture selection
  directly through the established US 1.0 configuration root.
- Preserve separate original revision progress, Controller Paks and immutable
  native save ownership, including modded/online route guards.
- Discover old US1.1 texture library entries read-only; avoid a second pack copy.
- Correct 4K UI framebuffer scaling and keep HDMI output separate from render scale.
- Reduce default driver logging and retain bounded high-refresh fallback.
- Keep original title icon and paired selection/launch artwork; release launch
  overlay after first presentation.
- Keep classic face-button gas tapping; remove the optional R2/L2 driving preset.
  Migrate only exact legacy preset matches, preserve custom bindings and Undo.
- Replace the native revision radio list with full-width US 1.0 / US 1.1 cards,
  distinct active/next-launch badges and unchanged save ownership.
- Optional dim held-balloon light colors through existing native SDL LED support;
  default off, shared setting, changed-color sends and bounded failure/cleanup.
- Private own journal, exact-build symbols, raw Klog/receive index and local report.
- Fresh-source dependency patch tests, actual shared config/texture/save-route
  checks and contained real-fault-to-source-line diagnostic acceptance.

Current physical results and experimental limits are in WORK_STATUS.md. This is
source only; native game outputs and ROMs are built privately by each user.

- Keep accepted high-refresh output for VRR-declared titles when idle vblank cadence varies; display the nominal mode honestly. Native RT64 Match display now follows PS5 output, not the SDL dummy display.
- Restore system light-bar ownership when no item is held, and remove the empty central widget from version cards.
