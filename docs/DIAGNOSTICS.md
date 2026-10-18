# Report a problem 🐛

Check the [known issues](KNOWN_ISSUES.md), then [open a report](https://github.com/drwellness666/DKR-R-PS5/issues).

Include:

- Port version and the active US revision shown under **Touchpad → Play**.
- PS5 firmware and homebrew setup.
- Short steps, expected result and what actually happened; include the exact error if one appeared.
- Relevant graphics settings and whether a texture pack or mod is active.

A photo or short DKR-R log excerpt can help. Remove personal paths and addresses. Do not attach ROMs, extracted assets, saves, texture packs or full console dumps. Logs are not uploaded automatically.

## Measuring a stutter

Under **Graphics → Renderer & performance**, select **Start 60-second frame-time capture**, return to the affected scene, then **Stop and save**. Include the summary and the scene you measured. Time spent in menus is included.

This measures presentation-call timing, not HDMI scanout or simulation speed. A TV reporting 120 Hz does not prove 120 rendered FPS.

Local session journals are stored under `/data/dkrr/diagnostics` when available. A kernel panic, GPU hang or power loss may leave no useful journal. If the game cannot start, report the visible message first.

[Developer debugging tools](DEVELOPMENT.md#debugging) · [Help](README.md)
