# Verification scope — integrated source beta

Current physical observations are in WORK_STATUS.md. CHECKS.json is the machine-
readable summary; raw console evidence and ROM-derived symbols stay private.

## Current local source checks

The current release preparation passed 16 targeted checks: actual controller
classic inputs/removed-layout migration/binding persistence/ImGui navigation; shared profile/library paths;
header-aware native identity; full fresh public dependency preparation;
production journals/signal recording; bounded own journal reader; actual Klog
relay/capture; combined saved-session report with real contained fault-to-source
attribution; 4K UI framebuffer handling; driver policy; update receipts; actual
file updater/rollback; installer; revision selection; performance summary; private build receipt generation.

Fresh preparation applies all 54 pinned dependency patches under an outer Git
repository, verifies their changes and reuses the complete saved stamp. It also
runs the actual shared-texture scan/settings/delete code and native guest-entry/
online-route checks on those fresh sources. The patch manifest covers 28 authored
upstream file changes. No ROM is needed for these source tests.

The GitHub workflow is exported from its actual checked-in file, not maintained
as a second handwritten list. It scans the source export, verifies SHA256SUMS
and runs ROM-free host checks. Source CI is separate from native game execution.

The new light-policy test exercises both actual US address tables, guest bounds,
read-only inventory ownership, pause/menu guards, atomic expiry and the actual
pinned SDL LED function with a modeled transport. It checks changed-color sends,
failed-call suppression, exact reset command rejection and native system reset.
The owner confirmed the held-item colors; the reset refinement is a separate check.
The previous exported 40-command source workflow passed after scoped network/socket
environment reruns. The new 41st check verifies the actual native display getter.
The VRR-specific test compiles the actual WSI open path and exercises positive
negotiation, idle variable cadence, unsupported outputs, configure refusal, failed
waits and the retained fixed-rate fallback.

## Native build and physical test

The latest local controller revision also passes those actual input/navigation
tests, fresh 54-patch preparation and full native build/link. It is installed with
authoritative update receipt and full executable hash read-back; first presentation
and exact legacy-map migration are logged. The owner confirms visible gameplay and correct Classic controls.
It removes the optional driving preset while preserving Classic/custom controls.

The accepted full native DKRPortGame target and FSELF link passed with private line tables
and both owned US payloads. The current eboot was installed through an exact-
predecessor update on 15 October 2026 and re-read with matching SHA256. The owner
confirmed normal image/audio, corrected full-size 4K settings, R2/L2 driving and
visible remastered textures on US 1.1. Further session acceptance is recorded in
WORK_STATUS.md rather than inferred from an install receipt.

The original console save/Pak protection remains strict: metadata uncertainty,
foreign ownership, unexpected aliases or route mismatches are refused. A shared
configuration root must not reconstruct a mutable save path at guest entry.
The original separate revision save/Pak roots, frozen context and read-back
checks remain in the patched entry functions.

## Reproduction

Use `.github/workflows/source-check.yml` for the standard source gate. Actual
shared-texture and guest-entry tests additionally accept a prepared GAME_WORK:

```bash
python3 tools/test_shared_texture_runtime.py "$DKR_GAME_WORK"
python3 tools/test_ps5_shared_save_entry.py "$DKR_GAME_WORK"
```

Hardware still determines actual HDMI cadence, physical vibration, save/reload,
visual issues and sustained performance. A loopback fixture, source CI, successful
link, nominal 120 Hz acceptance or disconnected socket does not establish these.

## Current VRR refinement

The combined build integrated-8f7c534368169906167740cc passes native link/FSELF
integrity, exact-predecessor update, full installed executable SHA256 read-back,
first presentation and splash release. With owner-confirmed VRR Automatic and
unsupported-games override Off, the PS5 AV log reports HDMI2160P11988, VRR48-120
and ALLM game. Its20.879-ms idle vblank sample now retains the accepted mode.
RT64 reports requested/effective/display120 in Match display mode. The owner subsequently confirmed normal picture/audio
and VRR active on the TV. The owner also confirms correct native light reset and disappearance of empty
card widgets. The final card-width correction uses the same page width as Return
to race; actual rendered ImGui edge and navigation tests pass at wide/narrow sizes.

The owner also confirms the final installed card-width refinement aligns both
edges with Return to race. The actual rendered geometry/navigation check is
now the 42nd command in the ROM-free source workflow.
