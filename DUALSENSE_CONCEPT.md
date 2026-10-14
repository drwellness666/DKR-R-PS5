# DualSense features for DKR-R: proposal and verified backend boundaries

Advanced effects below remain a concept/source audit. The separate next controls
candidate implements an **optional conventional R2/L2 driving preset**, per-player
selection/undo and PlayStation binding labels; see [the controls guide](CONSOLE_FEATURES.md#optional-dualsense-driving-layout).
Its actual input/settings/UI tests and native build pass locally; physical PS5
acceptance is pending. The published beta.1 snapshot remains unchanged. No new
controller-effect API is called by either candidate. The public homebrew
SDK and pinned SDL source are available here; the licensed official Sony console
SDK is not part of this repository or this assessment.

## Inventory and current evidence

Sony documents adaptive L2/R2 resistance, fine haptic feedback, touchpad, motion
sensing, light bar/player indicators, speaker, microphone/mute, headset connection,
USB/Bluetooth and ordinary buttons/sticks. Hardware support does not imply that
this port's native backend supports every feature. [Sony feature overview](https://www.playstation.com/en-us/accessories/dualsense-wireless-controller/),
[controller manual](https://www.playstation.com/content/dam/global_pdc/en/corporate/support/manuals/accessories/ps5-accessories/dualsense-wireless-controller-cfi-zct1w/CFI-ZCT1W_PS5_Wireless%20Controller_Instruction_Manual%24en.pdf).

| Feature | Evidence in this port/toolchain | Useful DKR-R proposal |
| --- | --- | --- |
| Buttons, sticks, analog L2/R2 | Actual native SDL axes/buttons; ordinary controls user-confirmed | Optional R2 accelerate/L2 brake preset implemented in the next local candidate; original layout retained |
| Ordinary vibration | SDL Rumble → scePadSetVibration; actual driver transport tested locally; physical qualification pending | Original Rumble Pak first, adjustable strength and immediate stop |
| Detailed DualSense haptics | Current driver exposes ordinary vibration, not waveform haptics | Distinct road bumps, water impacts and propeller/turbulence once a qualified actuator backend exists |
| Adaptive triggers | scePadSetTriggerEffect/Strength names exist in SDK link stubs; native SDL effect/trigger functions return unsupported | Gentle throttle resistance; brief boost pulse; braking/drift feedback |
| Gyro and accelerometer | Native PadData describes orientation/velocity/acceleration; SDL does not register/publish sensors, and enabling returns unsupported | Optional fine steering for kart/hovercraft; pitch/roll for aircraft, with recenter and stick fallback |
| Touchpad click | Mapped to Back in this port; overlay user-confirmed | Keep one predictable menu toggle |
| Touchpad contacts/swipes | Two contacts described in PadData; current driver does not publish touch events | Optional swipe between pages; retain D-pad and L1/R1 |
| RGB light bar | Concrete SDL SetLED → scePadSetLightBar; game effects not implemented | Optional balloon/item color or player identification; subdued boost/finish cue |
| Player indicator LEDs | System feature; no qualified app setter in this integration | Keep system player ownership; only add after a documented controllable path |
| Controller speaker/headset | Current SDL driver opens ordinary system MAIN audio, not a qualified controller-speaker route | Optional short pickup/countdown effects, with TV/headset mix and volume control |
| Microphone and mute | No capture/backend integrated; no input is recorded | Voice chat only as a separate opt-in feature; do not make sound/blowing required for play |
| PS/Create buttons | System actions, not ordinary extra game inputs here | Preserve system menu/capture behavior |
| Battery, USB/Bluetooth | Managed by console; no reliable battery telemetry published by current native SDL backend | Optional indicator only after valid data; do not invent a battery percentage |

The exact pinned [SDL PS5 driver](https://github.com/ps5-payload-dev/SDL/blob/ee4c47dc0d617b3bc8f35108f9956baf228a1322/src/joystick/ps5/SDL_ps5joystick.c)
implements buttons/axes, LED and ordinary rumble. Its trigger-rumble, raw-effect
and sensor-enable functions return `SDL_Unsupported()`; its advertised capabilities
are LED and rumble. The [data header](https://github.com/ps5-payload-dev/SDL/blob/ee4c47dc0d617b3bc8f35108f9956baf228a1322/src/joystick/ps5/SDL_ps5joystick.h)
describes motion/touch fields, but describing them does not publish SDL events.
These native limitations explain why an upstream gyro option is not proof of
working gyro on the PS5 port. The [SDK stubs](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b83202be73e930050e00de0f1b4d9ec46c0391bf/sce_stubs/libScePad.c)
name advanced APIs without giving a complete, qualified effect contract.

## Proposed adaptive-trigger behavior

The following are design proposals, to be validated physically rather than
advertised as existing support:

- **Kart:** light R2 throttle resistance, a short softer pulse for boost and a
  progressive L2 brake feel. Drift can add a small cue, without blocking release.
- **Hovercraft:** softer smooth throttle resistance; brief pulses for water impact,
  with less sustained resistance than the kart.
- **Aircraft:** very light throttle resistance, a short boost cue and occasional
  restrained turbulence feedback. Steering remains available independently.

The original N64 accelerator/brake actions are digital. Mapping a pressure-sensitive
trigger to them should preserve the original action threshold/physics unless a
separate, explicitly chosen gameplay modification is developed. Trigger resistance
is feedback; it does not by itself add analog acceleration to DKR.

An existing Cross-to-accelerate mapping also means the player may not be touching
R2. A trigger-feedback feature therefore needs an optional, clearly labelled
control preset rather than permanently changing the existing layout. Original
controls must continue to work. Both US revisions should receive the same host
feedback policy, while their original version-specific game behavior remains intact.

Publicly published Valve controller interfaces describe trigger effect categories
and a defined command layout, which is useful research evidence. It is a PC/Steam
interface and does not qualify the native PS5 call/handle/ABI on firmware 13.60.
The copyrighted header is not copied into this source export.
[Valve published interface](https://github.com/ValveSoftware/Proton/blob/proton_11.0/lsteamclient/steamworks_sdk_155/isteamdualsense.h).
Do not substitute PC USB/Bluetooth HID packets for a native ScePad call or guess
an unknown call signature merely because a function name resolves.

## Gyro integration plan

DKR-R already has gyro settings, calibration/recenter, per-vehicle axis choices
and optional stick blending. The missing native bridge should provide timestamped
SDL gyro/acceleration samples through the **existing owned controller handle**.
It should not open a second competing input loop.

First verify sensor-enable/reset signature, structure layout, units and axes from
public implementation/documentation; then use a focused native sample capture
with neutral/stationary and known rotations. Convert to the SDL units and only
advertise capability when real samples are valid. Missing/invalid/disconnected
samples fall back to ordinary stick input. Recenter on focus changes; disable
motion steering while navigating the interface. Add per-vehicle sensitivity,
deadzone, inversion and a quick recenter gesture without consuming the menu button.

## Architecture and performance requirements

Feed a small authored host-feedback layer from validated game events and existing
controller ownership. Candidate event data: vehicle class, speed/boost, braking,
collision, item change and pause/focus. Access must use reviewed revision-aware
runtime bindings; no guessed guest address or unchecked host pointer.

The feedback layer should keep only the newest continuous state per player and
bounded short one-shot events. Use changed-state sends plus a limited refresh
rate; do not allocate, access files or emit a log every frame. The renderer must
never wait for a controller effect. One user's disconnect/failure disables their
extra effects, while core controls and the game keep running.

Provide **Off / Subtle / Standard** with independent sliders/toggles for vibration,
trigger effects, gyro, light and speaker. Default new effects off until qualified.
Always release resistance and vibration on pause, menu/focus loss, disconnect,
restart/exit and errors, using a verified reset path. Respect system accessory
settings and avoid forceful effects that make full travel/release difficult.

Do not derive waveform feedback from a permanent extra audio capture/mix in the
first implementation. That would increase audio-routing work before current
microstutter and sound behavior have been qualified. A later waveform backend
must prove its channel/port format and independent stop behavior.

## Suggested order and acceptance gates

1. Qualify the existing ordinary Rumble Pak route, including strength and exit stop.
2. Connect native gyro samples to the existing upstream settings, after ABI/unit
   proof; use synthetic mapping tests and physical stationary/rotation/recenter tests.
3. Add optional LED cues through the concrete existing driver path.
4. Prototype adaptive triggers separately: verified ABI/handle, off/reset first,
   then one mild effect, controller-settings behavior and menu/disconnect cleanup.
   Only then add vehicle/game-event mapping.
5. Qualify detailed haptics and controller speaker audio separately. Microphone
   capture/voice chat stays a separate optional project feature.

Every stage needs native error handling, source/licence provenance and a matched
performance comparison. Passing a link stub, API return or synthetic test is not
proof that resistance, gyro units or controller sound feel/behave correctly.
