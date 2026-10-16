# Install DKR-R on PS5 📦

The Beta 3 installer is implemented and builds for PS5. Its end-to-end console
run is still pending; use it as an experimental preview.

## What you need

- A PS5 that already runs native homebrew, with an ELF receiver and ShadowMountPlus.
- Both owned, clean **US 1.0 and US 1.1 / Rev A** ROM dumps.
- A USB drive readable by the PS5. z64, n64 and v64 byte orders are supported; PAL is not.

## Four steps

1. Download **DKRR-Installer.elf** from this project's release.
2. Copy both ROMs into **DKRR** at the root of the USB drive. Keep them as ordinary files, not inside a ZIP. Names do not matter.
3. Connect the drive, start the normal homebrew environment and close DKR-R. Send the ELF using your usual sender on Windows or Linux.
4. Wait for the completion notification and ShadowMountPlus' app discovery, then launch the PS5 icon.

The installer identifies the ROMs by checksum and copies them to internal
storage. It generates controller sounds locally and registers the app folder.
Already installed, valid ROMs are reused. The PC and USB are no longer needed
for normal play; the usual homebrew environment is still required.

Optional Python sender, from an extracted source folder:

```console
python tools/send_elf.py YOUR_PS5_IP /path/to/DKRR-Installer.elf
```

Use `python3` on Linux. Python is not required when using another ELF sender.

## If installation stops

A missing-ROM message means one of the two clean US revisions was not found.
Unknown or changed installed files are preserved. After an interruption, rerun
the same ELF before launching the game; it can resume known partial work and
recover a staged update. Do not delete save folders to fix an installation.
The receiver's success marker is `DKRR UNIVERSAL INSTALL PASS.`.

[Updates](UPDATES.md) · [Controls](CONTROLS.md) · [Test status](VALIDATION.md).
