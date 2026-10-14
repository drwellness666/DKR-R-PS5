# Optional native PS5 display features

Reviewed 2026-10-14 against the pinned public homebrew sources, not a licensed Sony SDK.
The owner's TV is an LG C1. This document distinguishes the title's buffers,
rendered game detail, presentation rate and the HDMI signal reported by the TV.
None of the new output choices is yet physically accepted on the owner's PS5.

## Existing settings and PS5 integration

Use **Graphics → Game display**. The existing **Resolution** control changes
internal game rendering; on PS5 it is labelled **Game render resolution**.
The existing **Presentation rate → Match display** follows the rate reported
by the PS5 display surface. **Manual target** remains an independent FPS target.
Accurate presentation retains its original 30 FPS behavior.

The desktop **Window mode** field is replaced on PS5 with **TV output resolution**:
1080p, 1440p or 4K (2160p). An explicit Apply saves the choice for the next title
launch. The current game does not restart or change output under the player.
**Restore 1080p / 60 Hz** saves the original output for the next launch.
The UI also shows the selected buffer size and nominal refresh; it does not
claim to have read the HDMI signal or the TV's VRR state.

This is buffer resolution, not a request to change global PS5 resolution.
VideoOut can scale a 1080p buffer into a 4K HDMI signal. Conversely, asking for
a 4K buffer does not prove a 4K HDMI signal if the system is set to 1080p.
For full rendered 4K detail, use Modern and the existing automatic render scale
or Quality preset as appropriate; changing output alone does not add detail to
an Original (240p) game image. 4K has four times the pixels of 1080p. Higher
resolution/refresh can increase load and is not a stutter fix.

Only Vulkan-enumerated modes with compatible planes are selected. An unavailable
120 Hz mode falls back to the same size at 60 Hz. An unavailable resolution or
refused optional surface can fall back to the original 1080p mode. Device loss
is an error; it is not retried. A later swapchain/allocation failure is not
covered by the surface fallback and still requires the recorded rollback.

## 120 Hz: separate optional build, then explicit runtime choice

The normal source build keeps the existing title metadata and 60 Hz default.
The experimental build must be requested explicitly before `build_game.sh`:

```bash
export DKR_PS5_HIGH_REFRESH_BUILD=1
bash build_game.sh
```

Use a **fresh game work directory** for the experiment. The optional build
adds the **TV refresh rate: 60 / 120 Hz** field in the same Game display card.
It still starts at 1080p / 60 Hz without a saved choice. A previous display
preference is honored; inspect it before comparing an existing installation.

The public pinned WSI checks the title's `attribute3` high-refresh declaration
(0x40), asks `sceVideoOutIsOutputSupported(15)`, configures only a positive answer,
then measures vblank intervals. Refusal or a failed/slow measurement restores
the 60 Hz path. The title's wrappers keep that request off unless the user chose
120 Hz, and restore the qualified 60 Hz mode on open in the capable test build
when 60 Hz is selected. They operate only within this title's linked process.

The capable build declares `attribute3 | 0x40040`, following the pinned public
WSI's description of the VRR-compatible high-refresh declaration. These are
community source/evidence, not a published Sony ABI guarantee for firmware 13.60.
The older 0x80000 declaration is not used. This metadata difference needs the
loader's normal title registration to agree with the app; an executable-only
update is insufficient. The existing metadata-preserving update helper deliberately
refuses this capable build. Do not bypass its refusal or edit system databases.
The capable bundle is held separately until the ordinary title/registration
path has been checked on the console. The ordinary 60 Hz candidate is the first
test for tonight; all three output resolutions are available there.

No 120 FPS gameplay guarantee follows from a 120 Hz HDMI mode. Select Modern and
the existing Match display control when comparing interpolated presentation;
the original game simulation, time and audio speed are unchanged.

## VRR and ALLM

These are optional PS5 system/TV settings; the current renderer has no qualified
application API to toggle either one or query its actual HDMI state. No working
VRR checkbox is invented. The graphics page explains where they are controlled.
VSync/FIFO remains enabled; disabling it is not how VRR is activated.

The C1 supports HDMI VRR/ALLM and 4K/120 Hz. For the evening comparison, use
PS5 **Settings → Screen and Video → Video Output**, examine the connected HDMI
device information, and compare VRR Off versus Automatic / Apply to Unsupported
Games only after the ordinary game picture/audio and matching output are stable.
Use the TV's Game Optimizer/HDMI signal information to record actual operation.
System permission for unsupported games is not proof that this title used VRR.
Sony documents that VRR output also engages low-latency mode even if ALLM is Off;
turning ALLM off completely requires turning VRR off too.

VRR can accommodate uneven frame delivery in its working range. It cannot
remove a CPU/load stall, shader compilation or an audio underrun. ALLM reduces
TV processing latency; it does not increase game rendering speed. Do not change
the console enabler/kstuff state to reproduce another project's performance claim.

## HDR

The pinned PS5 `wsi_common_videoout.c` advertises only B8G8R8A8 UNORM/SRGB,
`SRGB_NONLINEAR` and FIFO. Its native pixel format is explicitly 8-bit SDR.
The generic Linux DRM display backend's HDR paths do not run on PS5. HDR export
names in an import stub are not effect/metadata structures or callable contracts.
RT64 high-precision intermediate framebuffers are not HDR TV output.

Native HDR needs a qualified 10-bit/float output format, output color transform,
tone mapping and VideoOut metadata/signal handling. They are absent here, so the
page reports SDR and supplies no misleading enable switch. Sony's system
**HDR → Always On** can convert SDR apps into an HDR signal; that is a system
conversion, not native HDR rendering. Use **On When Supported** for the SDR
baseline. True HDR remains a backend development task with a separate future
test; no guessed `sceVideoOutSys*` call is used.

## Storage and offline validation

Only `/data/dkrr/preferences/display-output.txt` is written, with exact bounded
keys `resolution=1080p|1440p|2160p` and `refresh=60|120`. Writes require ordinary
own directories, recognized prior contents and an unused temporary filename.
They use exclusive/no-follow creation, write/fsync and rename. Invalid, special,
symlinked or collision inputs are refused; saves, profiles and ROMs are untouched.
Apply changes the pending choice; the active startup choice remains fixed.
The graphics page reads preferences on entry, not every frame. The driver gate
reads already initialized memory, not files during presentation.

ROM-free ASan/UBSan tests exercise actual preference writes/refusals, the native
gate with modeled VideoOut APIs, the actual Vulkan surface adapter with modeled
enumeration/refusal/device-loss paths, and the actual native UI fields with
modeled drawing helpers. Drawing/final TV timing and driver allocation remain
physical checks. Reproduce with `python3 tools/test_ps5_display.py`; repeated
`--cache ABSOLUTE_DOWNLOAD_CACHE` avoids fetching the hash-pinned public headers.

Standalone GPU/RT64 probes keep their existing default and acquire no game
preference references. The game CMake target alone enables the display choice.

## Primary sources

- [Sony PS5 output settings and ALLM behavior](https://www.playstation.com/en-us/support/hardware/ps5-4k-resolution-guide/).
- [LG C1/G1 gaming capabilities](https://www.lg.com/in/oled-tvs/2021/gaming/).
- [Pinned native VideoOut WSI](https://github.com/mihawk-99/PS5_Mesa/blob/47e9eb61ee7e1112194557e1e047178562827bc6/src/vulkan/wsi/wsi_common_videoout.c).
- [Pinned qualified public VideoOut declarations](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b83202be73e930050e00de0f1b4d9ec46c0391bf/platform/include/ps5platform/videoout.h).
- [Upstream measured mode/driver findings](https://github.com/mihawk-99/PS5_Vulkan/blob/main/docs/HARDWARE_FINDINGS.md).
