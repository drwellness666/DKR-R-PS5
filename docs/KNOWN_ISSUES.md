# Known issues and compatibility ⚠️

## Known problems

| Problem | Details |
|---|---|
| Large texture packs | Conversion and verification can take time. Keep DKR-R open until installation completes; copy/verification progress is displayed. |
| Menu stutter when scrolling up | In the launcher, scrolling up can stutter briefly. The cause is not found yet. |
| Water flickers on first load | The first time water appears after starting the game, it can flicker briefly. The cause is not found yet. |

## Tested setup

Version 1.2.2 is based on **DKR-R 1.0.15** and opens the launcher first. Version 1.2.1 was tried on a PS5 and several problems found there (lobby chat with no gamepad shortcut, a few menu-text/layout details, a duplicated About entry) are addressed in 1.2.2. Those fixes were checked on a PC and still need their own test on the console. Please report anything that worked in 1.2.1 and no longer does.

For version 1.0 (DKR-R 1.0.9), PS5 sessions confirmed installation from owned USB ROMs, both US revisions, save/load after a full console restart, shared settings and texture choices, classic controls, vibration, balloon lighting, gyro, adaptive L2 and controller sounds. 4K/119.88 Hz, VRR Automatic and neutral HDR10 output were confirmed on an LG C1.

The tested console used **firmware 13.60, Relapse / WebKit Autoloader 0.6.2, kstuff-lite and ShadowMountPlus**. This describes the test setup; it is not a firmware-upgrade recommendation.

Sherbet Land, Moggy and Sr.gu’s HD Texture Pack 0.2.8 installed through the Mod Store. Compatibility is not guaranteed for every mod or combination. A custom-character Hubworld freeze was reported during testing; the subsequent record-file startup refusal was fixed. Report any repeat with the character, location and graphics settings.

The online directory, Discord connection and lobbies were exercised on PS5. Full races between two physical consoles have not yet been confirmed in these sessions. Host checks and lobby access do not substitute for that test.

The in-game updater only installs signed releases and keeps your data. If an update ever fails with "Download cancelled or verification failed", simply try **Touchpad → Updates** again; your installation stays unchanged. **DKRR-Update.elf** remains the offline alternative. Other loaders and displays, ghosts/Paks and the Windows/WSL2 build route have had little testing. A constant 120 rendered FPS is not guaranteed.

[Report a problem](https://github.com/drwellness666/DKR-R-PS5/issues) · [What to include](DIAGNOSTICS.md) · [Planned improvements](ROADMAP.md)
