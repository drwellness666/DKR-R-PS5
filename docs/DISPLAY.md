# Display and performance

[Controls](CONTROLS.md) · [Known issues](KNOWN_ISSUES.md) · [Diagnostics](DIAGNOSTICS.md)

## Output size and rendered detail

Under **Graphics → Game display**, **TV output resolution** selects a 1080p,
1440p or 4K output buffer. Apply the choice and restart the game normally.
**Game render resolution** controls internal detail separately. The settings
interface keeps its normal screen-filling size at 4K.

A larger output buffer does not add detail when the game is internally rendered
at its original resolution. Higher render resolution, anti-aliasing and texture
packs increase graphics load. Start with the default settings if performance
is uneven. **Restore 1080p / 60 Hz** applies to the next launch.

## Experimental 120 Hz and VRR

The capable build offers **TV refresh rate: 60 / 120 Hz**. Apply and restart.
**Render frame-rate target → Match display (default)** follows the active native output;
output Hz and rendered FPS are different measurements. Accurate presentation
retains the original timing behavior.

On PS5, keep **120-Hz Output: Automatic** and **VRR: Automatic**. The global
**Apply to unsupported games** option is not required. A 4K/119.88-Hz signal with
VRR active was confirmed on an LG C1. Other displays and firmware/loader
combinations still need testing. Check the TV's signal information for actual
HDMI/VRR status; the in-game output-buffer line does not read the TV.

Unsupported modes can fall back to 60 Hz or the original output mode. A GPU/device
failure is not silently retried. If a newly selected setting prevents a usable
start, preserve the configuration and use the [diagnostic guide](DIAGNOSTICS.md).

ALLM is governed by the console/TV output path. It was reported as game mode in
the tested system log; there is no separate ALLM override in this port. Output
is SDR. HDR is not implemented.

## Frame-rate target

For Modern presentation, a fresh PS5 configuration defaults to **Match display**:
60-Hz output targets 60 rendered presentations, 120-Hz output targets 120. Native
fallback to 60 Hz also changes that target to 60. A saved manual target remains
intentional; it can reduce GPU load when render detail is too expensive.

RT64 interpolates the original 30-Hz visual states. Game simulation and speed are
not doubled. Accurate keeps the original 30-FPS presentation. Output Hz, renderer
target and measured presentation-return rate are separate values; no setting
guarantees sustained 120 FPS in every scene. **Automatic integer scale** changes
internal render resolution, not the frame-rate target.

## Icon, home-menu background and launch image

The build uses the upstream DKR-R icon and cloud/wordmark background. The same
background is used for launch, and the overlay clears after the first game
presentation. Icon, background and normal handoff were confirmed on the test PS5.
Artwork sources and terms are recorded in [ARTWORK.md](../homebrew/DKR-R-Game/ARTWORK.md).

A fresh install requires ShadowMountPlus to scan `/data/homebrew`. Copying artwork
files alone does not guarantee that the PS5 shell refreshes an already registered
title. Compatible updates preserve existing title identity and artwork.
