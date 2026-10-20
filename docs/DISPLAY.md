# Display and performance 📺

Open **Touchpad → Graphics → Game display**. Apply output changes and restart the game normally.

| Setting | What it changes |
|---|---|
| TV output resolution | The output buffer: 1080p, 1440p or 4K |
| Game render resolution | The detail rendered inside the game; higher settings cost more performance |
| TV refresh rate | 60 Hz or experimental 120 Hz |
| Render frame-rate target → Match display | In Modern mode, follow the active 60/120-Hz output |
| HDR output | Optional HDR10; SDR is the default |

The menu keeps its normal screen-filling size at 4K. A 4K output alone does not increase the game's internal detail. **Automatic integer scale** controls internal resolution, not frame rate.

## 120 Hz and VRR

On PS5, use **120-Hz Output: Automatic** and **VRR: Automatic**. **Apply to unsupported games** is not required. A 4K/119.88-Hz signal with VRR was confirmed on an LG C1; check your TV's signal information for actual HDMI status.

**Match display** is the default Modern rendering target. It follows a fallback to 60 Hz too. Higher visual frame rates interpolate the original game states; they do not speed up physics or race timers. Accurate presentation keeps the original 30-FPS behavior. Sustained 120 FPS depends on the scene and graphics settings.

If performance is uneven, start with default render settings, lower anti-aliasing and try without a texture pack. **Restore 1080p / 60 Hz** takes effect on the next start. ALLM follows the console/TV game-mode settings; there is no separate override.

## Optional HDR10

Enable **HDR output → HDR10** with HDR enabled on the PS5 and a compatible display. The game can fall back to SDR if HDR output is unavailable.

**HDR brightness** adjusts menu/reference white, display peak and highlight expansion. **Use neutral HDR mapping** restores reference white and removes added highlight expansion. Use the calibration pattern to suit your display; it disappears when you leave the page. Defaults are 200-nit reference white, 800-nit peak and 0% highlight expansion.

The original game was SDR. This feature expands that image for HDR output; it is not an authored HDR remaster. Physical brightness, performance and HDR with VRR/120 Hz remain part of the [feature checks](KNOWN_ISSUES.md#feature-status).

[Controls](CONTROLS.md) · [Performance reports](DIAGNOSTICS.md)
