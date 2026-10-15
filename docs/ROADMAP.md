# Roadmap

## Next controller features

- **Optional gyro steering:** calibrated motion steering, recenter, adjustable sensitivity and immediate stick fallback. First implement and verify native PS5 sensor transport; upstream gyro settings alone are insufficient.
- **Optional adaptive L2 weapon effect:** a small resistance point and a short impulse tied to the actual item/shot event. Reset on release, empty inventory, pause, disconnect and when disabled. Keep Classic Cross acceleration and gas tapping.

**Deutsch:** Als Nächstes sind optionale **Gyrosteuerung** und ein **adaptiver L2-Schusseffekt** vorgesehen. Beide müssen abschaltbar bleiben und dürfen die klassische Steuerung nicht beeinträchtigen.

## Stability and usability

- Isolate the brief Tracks-preview water flash.
- Investigate reproducible frame-time spikes and texture/graphics combinations.
- Expand testing across controllers, displays, firmware/loaders, Controller Paks and longer sessions.
- Improve the local build assistant and incremental updates from tester feedback.

There are no promised dates. The beta already includes optional balloon light colors, system-light reset and experimental 4K/120-Hz output with VRR. [Current limitations](KNOWN_ISSUES.md).
