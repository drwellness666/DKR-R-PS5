# Standalone PS5 installer: target and current status

The intended end-user flow is:

1. Send one ROM-free installer ELF to the homebrew-ready PS5.
2. Select your own supported ROM from console/USB storage with the controller.
3. Validate and import that ROM into DKR-R's own storage, then finish installation.
4. Launch the installed game from its PS5 icon, without a running PC.

This is a development target, **not the current downloadable installer**. The
current release distributes source and builds on a Linux PC/WSL2. Its generated
native game runs on the PS5; the PC is not emulating or streaming it.

## What already exists

The project has a native game, a ROM-free application installer builder, separate
owned-ROM validation/canonicalization, title files and an upstream controller ROM
browser. The accepted game still expects its canonical ROM at startup. Its
working in-game overlay does not by itself prove that the upstream pre-game
launcher renders correctly on the native PS5 display path.

## Work needed

- Show a native first-run/import screen before a ROM is installed.
- Expose console/USB locations, validate contents and byte order, then copy the
  selected ROM atomically to the owned destination. Reject corrupt/unsupported
  images and existing different files; never depend on the USB remaining inserted.
- Persist the selected revision and reopen the normal game after installation.
- Start required save support from the console workflow without a PC or separate
  per-boot user command; preserve the existing file-ownership/alias checks.
- Verify installation, cold boot, save/reload and launching with the PC disconnected.

The installer can contain a precompiled native runtime. It does not need to run
LLVM or compile the whole port on the PS5. A build-on-console product would be a
different toolchain project and is not required for this target.

## Distribution

Upstream explicitly discusses binary releases of its combined runtime. Such a
release must preserve the GPL notices and provide complete corresponding source
for the exact shipped revision/dependencies. A supported ROM remains a separate
user input. Font/artwork/dependency terms must also be checked for the actual
installer contents. No blanket permission for original game data is inferred.

[Upstream runtime notice](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/runtime-recomp/COPYING-NOTICE.md)
· [Distribution notes](LICENSES.md) · [Save support today](SAVES.md)
