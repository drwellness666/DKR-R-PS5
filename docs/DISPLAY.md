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
**Presentation rate → Match display** follows the selected native output;
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

## Icon, home-menu background and launch image

The build uses the upstream DKR-R icon and cloud/wordmark background. The same
background is used for launch, and the overlay clears after the first game
presentation. Icon, background and normal handoff were confirmed on the test PS5.
Artwork sources and terms are recorded in [ARTWORK.md](../homebrew/DKR-R-Game/ARTWORK.md).

A fresh install requires ShadowMountPlus to scan `/data/homebrew`. Copying artwork
files alone does not guarantee that the PS5 shell refreshes an already registered
title. Compatible updates preserve existing title identity and artwork.
