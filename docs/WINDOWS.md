# Windows

The [universal ELF route](INSTALLER.md) needs an ordinary ELF sender and USB ROMs;
it needs neither WSL nor a compiler. The instructions below are the optional
**developer source build** and its optional local build assistant.

Use an **x86-64 Windows 10 build 19041+ or Windows 11** PC. Extract this version's
source ZIP, then double-click **Start-Windows.cmd**. Windows Python is not needed.

The starter uses **Ubuntu 24.04 in WSL2**. If missing, it offers Microsoft's WSL
installation; only that step requests administrator access. Follow any Windows
restart and Ubuntu user-creation prompts, then start the same file again. It does
not change your default WSL distribution or build as root.

Verified public sources are staged in the Linux home filesystem for build speed;
ROMs and previous work are excluded from that copy. Missing Linux build packages
are listed and installed only after confirmation. The local browser assistant
then takes **both US ROMs**, builds one private installer and sends it to your PS5.
If no browser opens, use the local address printed in the terminal.

Full Windows/WSL2 build-to-console acceptance remains pending. PowerShell syntax
or a Linux build alone cannot establish that result. Windows ARM and macOS are
outside this version's scope.

Microsoft: [WSL installation](https://learn.microsoft.com/en-us/windows/wsl/install),
[command reference](https://learn.microsoft.com/en-us/windows/wsl/basic-commands),
[Linux filesystem guidance](https://learn.microsoft.com/en-us/windows/wsl/filesystems).
