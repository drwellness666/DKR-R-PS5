# Pre-release review — 18 October 2026

**The current source is preparation for 1.0, not a release approval.** This
review checks implementation, failure handling, packaging and player guidance.
The follow-up [architecture review](CONCEPTUAL_REVIEW.md) records decisions and
implemented fixes for the design items below before final console qualification
and publication. No console installation was performed
for this review.

## Findings fixed in this review

| Finding | Result |
|---|---|
| An interrupted display, HDR or game-version preference write could block later changes | All three adapters exclusively reserve a fresh temporary slot, preserve occupied entries and synchronize the destination directory after replacement. |
| Game-version selection could block on a named pipe | Preference reads are nonblocking and require a regular file. |
| A crowded earlier USB drive stopped discovery of ROMs on later drives | A bounded search miss continues to the next drive; the `DKRR` folder remains the recommended location. |
| Unrelated 12-MiB files were read in full during ROM search | Validate the four-byte byte-order header before allocation and bulk reads; checksum verification remains mandatory. |
| Documentation contradicted the implemented HDR path and promised an unqualified older-beta upgrade | Player guides and architecture now describe the actual implementation and the metadata upgrade limitation. |
| CI used a deprecated Node runtime and retained checkout credentials unnecessarily | Pin checkout v7.0.1 by full commit, disable credential persistence and bound job duration. |

Regression tests exercise the actual adapters/installer. The new preference test
fails against the previous implementation; both USB regressions also fail
against the previous installer. The current installer suite has 20 cases.
Clang's static analyzer reports no diagnostics for the six installer C units and
six C++ update, speaker and preference units examined. This is limited static
evidence, not proof against all races or hardware failures.

## Release gates

| Priority | Open item | Required outcome |
|---|---|---|
| Release blocker | Older registered beta → HDR-capable 1.0 title metadata | Define and qualify an app-specific migration through the supported registration path. Both installers deliberately refuse conflicting registered metadata today. Do not bypass that guard or edit app databases/cache files by hand. |
| Release blocker | Distribution of fonts, title images and combined program | Complete the matching-source/component review and record the applicable redistribution terms. Original DKR-R typography remains unchanged; a project-code licence alone is not an artwork/font permission. |
| Release blocker | Final physical-console qualification | One exact build must cover fresh installation, upgrade/recovery, restart saves, output modes and the optional controller features. Earlier gameplay confirmation does not cover later installer/HDR changes. |
| Before publishing | Signed channel and downloadable assets | Freeze one build, source archive, notices and checksums. Sign its exact sequence/tag/package only after qualification; publish the channel after its matching assets. Keep signing keys outside the repository. |

## Questions for the conceptual audit

1. **One display transaction — implemented.** Output size/rate and HDR now use
   one versioned record. Apply and Restore replace the full combination;
   first-save migration preserves valid legacy preferences. Failure and
   process-cut tests cover retry and refusal without stale fallback.
2. **Update schema evolution.** Native updates intentionally accept exactly the
   reviewed 37 program paths plus the owner's 64 retained audio cues. New paths,
   removed files, runtime ABI changes and title metadata changes need a qualified
   upgrade route. Design that route before promising every future update can
   use the existing in-game client.
3. **Storage budget and recovery retention.** Native updates hold the download,
   staged files and recovery copies concurrently. A space estimate is now shown
   in the existing update card and a measured shortage stops before download.
   Manual history and session journals are retained; no automatic deletion of
   saves, unknown files or old recovery versions is introduced.
4. **App lifecycle.** Native input uses a dummy SDL window and treats it as
   focused. Verify PS-menu, suspend/resume, controller reconnect and shutdown
   behavior before adding automatic pause or effect cancellation through public
   system events. Existing menu/loading gates are not a lifecycle implementation.
5. **Installer feedback.** Notifications/log output keep the installer small.
   A future native progress/error view could show which ROM was found, available
   storage and registration status. Keep one installer and both required ROMs;
   avoid a second launcher or a PC runtime service.
6. **Measured performance.** Keep Tracks-preview water, shader compilation,
   presentation pacing, audio underruns and graphics load as separate hypotheses.
   Compare the same track/settings with and without the texture pack. The
   existing owned shader cache stays opt-in until driver-specific persistence
   and frame-time measurements justify a default change.

## Coverage and remaining compatibility

| Area reviewed | Current boundary |
|---|---|
| Pinned DKR-R/RT64 integration | Original typography and game timing are retained; platform hooks and reproducible patches remain separate from generated CPU code. |
| Graphics | Native VideoOut, 60/120-Hz negotiation, VRR, SDR and optional 10-bit HDR composition exist. Physical HDR brightness, fallback and GPU cost remain open. |
| DualSense | Classic controls, lights, gyro, adaptive L2 and bounded speaker events share the existing menu. Physical gyro/trigger/speaker behavior and multiple controllers need qualification. |
| Storage | Shared configuration/textures; separate revision progress/Paks; native save transactions and guarded recovery. Cold-boot and ghost/Pak acceptance remain open. |
| Internet updates | Native HTTPS, signed bounded data package, restricted origins, own-app lease, joined workers and startup recovery; no receiver dependency. Private releases cannot be downloaded anonymously. |
| Packaging | ROMs and decoded controller audio are excluded from universal packages. Program-file readback, native build receipts and source checksums bind the outputs. |
| Windows/Linux | The player route needs only USB ROMs and an ordinary ELF sender. WSL2 is the optional Windows source-build route; its complete Windows walkthrough remains unqualified. |
| Mods/online | Upstream features are retained with native restrictions; arbitrary mods, online modes and texture packs are not broadly qualified. |

No claim is made that the Tracks-preview water flash, occasional microstutters
or reported new-Adventure intro skip has been fixed.

## Public sources checked

- [Public payload SDK](https://github.com/ps5-payload-dev/sdk): toolchain and ELF-loader workflow; its sample test target uses a real console.
- [PS5 Vulkan/RADV](https://github.com/mihawk-99/PS5_Vulkan): current platform, presentation and pipeline-cache work. Review changes against the pinned revision before adopting an upgrade.
- [ShadowMountPlus API](https://github.com/drakmor/ShadowMountPlus/blob/1.7/docs/api.md): version/capability discovery, scan state and manual-source registration. Future integration should probe capabilities; it must not introduce a required LAN API or use the private ShellCore socket.
- [Kodi PS5 VideoOut](https://github.com/VivaLaVent/kodi-ps5/blob/main/overlay/xbmc/platform/ps5/VideoOutInfo.cpp): public output-status ABI and HDR status interpretation.
- [DKR-R asset policy](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md): original content/font provenance and redistribution boundaries.
- [Pinned checkout release](https://github.com/actions/checkout/commit/3d3c42e5aac5ba805825da76410c181273ba90b1): updated CI action; its action definition uses Node 24.

[Test status](VALIDATION.md) · [Known issues](KNOWN_ISSUES.md) ·
[Installation](INSTALLER.md) · [Updates](UPDATES.md).
