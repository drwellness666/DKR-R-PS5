# Windows source builds

**Installing a release does not require WSL, Python or a compiler.** Use the [installer ELF](INSTALLER.md) with an ordinary ELF sender. These instructions are only for compiling your own copy.

1. On **x86-64 Windows 10 build 19041+ or Windows 11**, extract the source ZIP and double-click **Start-Windows.cmd**.
2. If needed, accept Microsoft's **Ubuntu 24.04 / WSL2** setup. Complete any restart and Ubuntu account prompts, then start the file again. This is the only step requiring administrator access.
3. Review missing Linux build packages, then use the local browser assistant to select both US ROMs and build your private installer.

Keep the terminal open while building. If the browser does not open, use the local address printed there. Windows Python is not needed: compilation runs inside WSL2. Sources are staged in its Linux filesystem for build speed; ROMs are not uploaded.

The generated ROM-containing installer is for your own console and must not be shared. The complete Windows/WSL2 build route has [limited test coverage](KNOWN_ISSUES.md#feature-status). Windows ARM and macOS are outside this version's scope.

[Manual build commands](BUILD.md) · [Microsoft WSL installation](https://learn.microsoft.com/en-us/windows/wsl/install) · [WSL filesystem guidance](https://learn.microsoft.com/en-us/windows/wsl/filesystems)
