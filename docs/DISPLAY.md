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
uses SDR by default. The development HDR10 path is described below.

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

## Optional HDR10

Under **Graphics → Game display**, choose **HDR output → HDR10** and apply
for the next launch. HDR is off by default. PS5 HDR and a compatible display
are required. Initialization refusal or a persistent non-HDR HDMI status
returns to SDR; the saved preference is retained.

**HDR brightness** contains menu/reference white (80–300 nits), display peak
(400–2000 nits) and scene highlight expansion. The menu is composed separately
so its white does not inherit the scene peak. A temporary checker pattern shows
reference white/black and peak/98% peak; it clears when the page closes.
Defaults are 200-nit reference white, 800-nit peak and 35% highlight expansion.

The original game is SDR. This is an explicit SDR-to-HDR expansion into a real
10-bit BT.2020/PQ output, not an authored HDR remaster. The existing SDR VI/UI
pipelines and game logic are retained. The extra two image layers and final
pass are allocated only for HDR; their GPU cost still needs console measurement.

The first public installer declares HDR capability from the start. No upgrade
from private preview metadata is required. Later updates preserve the registered
title declaration; conflicting titles are refused. HDR/VRR/120-Hz coexistence
and display brightness remain to verify on PS5.
