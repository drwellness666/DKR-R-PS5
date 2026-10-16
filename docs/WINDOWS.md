# Windows

**Python is available for Windows**, but Python alone cannot build this port.
The current recipes also use Bash, Make, ELF/Linux host tools, LLVM, Mesa and
Linux shader compilers. Running `py setup.py` in Windows does not supply them.

## Build through WSL2

On an x86-64 Windows PC, use Linux under **WSL2**. In an Administrator PowerShell:

```powershell
wsl --install -d Ubuntu-24.04
```

Follow Windows' setup/restart prompts, then open Ubuntu. Confirm WSL version 2
with `wsl --list --verbose` in PowerShell. Extract the source ZIP into your Linux
home directory, for example `~/DKR-R-PS5`; avoid compiling under `/mnt/c`.

In that extracted folder **inside Ubuntu**:

```bash
python3 setup.py
```

The assistant lists missing Linux packages and the installation command. Install
those in Ubuntu and recheck. Select your own supported ROMs in the assistant.
Its files and compiler processes run locally, not on GitHub or the PS5.
If no browser opens, copy the printed `http://127.0.0.1:.../#...` address into your
Windows browser; keep the Ubuntu terminal open. Do not share that per-run address.

The Linux build is tested. WSL2 follows the same recipes but has not received a
complete build/install acceptance test for this release. Native Windows, Windows
on ARM and macOS builds are not currently supported.

Microsoft: [Install WSL](https://learn.microsoft.com/en-us/windows/wsl/install),
[Linux filesystem guidance](https://learn.microsoft.com/en-us/windows/wsl/filesystems).
Python: [Using Python on Windows](https://docs.python.org/3/using/windows.html).

## Sending an already-built payload from Windows

The small sender itself uses standard Python sockets and can run on Windows:

```powershell
py tools/send_elf.py YOUR_PS5_LAN_IP C:\path\to\your-private-installer.elf
```

That only transfers an existing local build; it does not compile the game.
Use the installer order and completion checks in [Build](BUILD.md).
