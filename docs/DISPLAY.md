# Display and performance 📺

Open **Touchpad → Graphics → Game display**. Apply output changes and restart the game normally.

| Setting | What it changes |
|---|---|
| TV output resolution | The output buffer: 1080p, 1440p or 4K |
| Game render resolution | The detail rendered inside the game; higher settings cost more performance |
| TV refresh & smoothness | 60 Hz or 120 Hz; Modern rendering follows the active output automatically |
| HDR output | Optional HDR10; SDR is the default |

The menu keeps its normal screen-filling size at 4K. A 4K output alone does not increase the game's internal detail. **Automatic integer scale** controls internal resolution, not frame rate.

## 120 Hz and VRR

On PS5, use **120-Hz Output: Automatic** and **VRR: Automatic**. **Apply to unsupported games** is not required. A 4K/119.88-Hz signal with VRR was confirmed on an LG C1; check your TV's signal information for actual HDMI status.

Modern rendering automatically follows the active display rate, including a fallback to 60 Hz. There is no separate manual FPS limit on PS5. Higher visual frame rates interpolate the original game states; they do not speed up physics or race timers. Accurate presentation keeps the original 30-FPS behavior. Sustained 120 FPS depends on the scene and graphics settings.

If performance is uneven, start with default render settings, lower anti-aliasing and try without a texture pack. **Restore 1080p / 60 Hz** takes effect on the next start. ALLM follows the console/TV game-mode settings; there is no separate override.

## Optional HDR10

Enable **HDR output → HDR10** with HDR enabled on the PS5 and a compatible display. The game can fall back to SDR if HDR output is unavailable.

HDR10 uses the neutral mapping confirmed during PS5 testing. The earlier highlight-expansion controls are removed. The original game was SDR; this is HDR output of that image, not an authored HDR remaster. Brightness also depends on your console and TV settings.

[Controls](CONTROLS.md) · [Performance reports](DIAGNOSTICS.md)
