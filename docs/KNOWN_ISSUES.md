# Known issues / Bekannte Fehler

## Water flicker in Tracks previews

A brief water flash can occur when entering a course preview in Tracks mode. The preview then displays normally. Water during Adventure gameplay was reported stable. The cause is not yet isolated; this beta does not claim a fix.

**Deutsch:** Beim Einstieg in die Streckenvorschau im Tracks-Modus kann das Wasser kurz flackern. Danach ist es normal; während Adventure-Fahrten wurde es als stabil bestätigt.

## Occasional microstutters

Occasional short stutters have been reported. One 60-second measurement at 60 FPS had no presentation-return gaps above 33 ms, but that does not cover every track, texture pack or graphics setting. Higher internal resolution and heavier anti-aliasing increase GPU load.

**Deutsch:** Gelegentliche Mikroruckler sind gemeldet. Eine unauffällige Messung ist keine Zusage für alle Strecken und Einstellungen.

## Save support after a console reboot

The accepted native setup uses a console-side save-metadata service. It is not
a PC connection and does not recover deleted saves. Manual restarts of that
service are still a limitation of the current installation flow; see
[Saves](SAVES.md) and the [standalone installer target](INSTALLER.md).

## Features and test limits

- 4K/119.88-Hz output with VRR Automatic was confirmed on one PS5/LG C1 setup. Leave “Apply to unsupported games” off. Other displays/loaders still need testing.
- Online play and arbitrary mods have not received console acceptance for this port.
- Adaptive triggers, native gyro, detailed DualSense haptics, controller speaker and HDR are not implemented. They are not working features merely because upstream has related settings.
- The reported Adventure-intro skip is not yet confirmed as a defect: the original new-game cinematic depends on a new save. Further reproduction is welcome.
- Virtual Controller Paks are present; dedicated ghost/Pak persistence needs wider testing. Normal Adventure save/reload was confirmed for both US revisions.

Report reproducible issues in this repository, with firmware/loader, port version, ROM revision and the steps. Do not send game files or full console dumps. [Roadmap](ROADMAP.md).
