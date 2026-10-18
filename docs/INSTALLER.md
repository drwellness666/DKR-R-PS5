# Install on PS5 📦

The final **1.0 installer** is in preparation. It installs the game on the console; no PC compilation is needed.

## Before you start

- Your PS5 must already run **native homebrew**, with an **ELF receiver** and **[ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus)**.
- Supply both of your own clean **US 1.0 and US 1.1 / Rev A** ROM dumps. `.z64`, `.n64` and `.v64` are accepted; PAL ROMs are not.
- Use a USB drive readable by the PS5. Leave the ROMs uncompressed.

## Install

1. Get **DKRR-Installer.elf** from this project's [releases](https://github.com/drwellness666/DKR-R-PS5/releases).
2. Create **DKRR** at the root of the USB drive and put both ROMs inside. File names do not matter.
3. Connect the drive and close DKR-R. Send the ELF using your usual Windows or Linux ELF sender.
4. Wait for the completion notification. Once ShadowMountPlus discovers the app, start **DKR-R** from its PS5 icon.

The installer checks and imports both ROMs and prepares controller sounds locally. After installation, the PC, USB drive and ELF receiver are no longer needed for play or in-game updates. Start your usual homebrew environment again after a console restart.

<details>
<summary>Optional Python sender for Windows or Linux</summary>

From an extracted source download, with [Python](https://www.python.org/downloads/) installed:

```console
python tools/send_elf.py YOUR_PS5_IP DKRR-Installer.elf
```

Use `python3` on Linux. Another ELF sender works too.

</details>

## If it stops

- **Missing ROM:** check that both clean US revisions are present as ordinary files in `USB/DKRR`.
- **Interrupted installation:** rerun the same ELF before starting the game. Keep the USB drive connected until completion.
- **Title conflict or changed installation:** the installer stops rather than replacing unfamiliar files. Report the exact message; do not delete saves.
- **Completed, but no icon:** check ShadowMountPlus discovery. Its completion is separate from the ELF transfer. The installer log's completion marker is `DKRR UNIVERSAL INSTALL PASS.`.

Version 1.0 uses a fresh installation; old development previews are not supported update targets.

[Controls](CONTROLS.md) · [Updates](UPDATES.md) · [Compatibility](KNOWN_ISSUES.md)
