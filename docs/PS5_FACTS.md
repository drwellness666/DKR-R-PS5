# PS5 facts and how they are tested

This file lists what is known about the console environment, with the evidence
level of each entry. `tests/ps5_env_shim.c` models these on a PC so that a bug
of this class fails in CI instead of on the console. A rule that is only a
hypothesis is a robustness test: the code must work whether or not it is true.

Evidence levels: **OBSERVED** (seen on the console, source named),
**CODE** (an earlier session recorded it in code or comments; not re-verified),
**HYPOTHESIS** (assumed, never observed).

| Rule (`DKRR_PS5_ENV`) | What the PC emulates | Level | Source |
|---|---|---|---|
| (not a rule) `/data/dkrr` belongs to root, mode 755; the title (uid 1) cannot create `tmp` there | `TMPDIR` must point into a profile folder | OBSERVED | FTP listing `drwxr-xr-x 0 0 /data/dkrr`, `drwxrwxrwx 1 0 profile-us-v77`; 1.2.4 fix |
| (not a rule) Journal files created with mode 600 are not readable over the console FTP (`550`) | Journals are created 0666 since 1.2.8 | OBSERVED | FTP `RETR /data/dkrr/diagnostics/run-*.log` |
| (not a rule) The storage service's unattended pass starts 10 s after launch; 1.2.4 closed 10-13 s after launch with it on | `startup_due` in `storage/service.cpp` | OBSERVED | file times: journal 22:34:26, process start 22:34:15.9 |
| (not a rule) These libc functions are **not exported** by the platform and stay bound to address 0; the first call crashes with `rip=0`: `readlink`, `symlink`, `link`, `fchown`, `mkstemp`, `strcasestr` (plus dormant `sceIme*`, `sceKeyboard*`, `sceNetPool*`) | `src/ps5_missing_libc.c` defines the six libc ones; `docs/IMPORTS_BASELINE.json` records all | OBSERVED | journal of 1.2.6: `DKRR_CRASH rip=0x0` after step `selftest proc`; `unresolved function import:` list at start |
| `nlink0` | regular files report `st_nlink == 0` | OBSERVED | `selftest.json` of 1.2.8: `stat file ... nlink=0`, also for folders |
| `uid0` | owner reported as root although the title runs as uid 1 | OBSERVED | `selftest.json` of 1.2.8: `uid=0 euid=1` on the title's own files |
| `proc` | `/proc` is absent | OBSERVED | `selftest.json` of 1.2.8: `stat /proc` -> `errno=2` (ENOENT); `readlink` itself is a stand-in returning `ENOSYS` |
| `flock` | advisory locks unsupported (`EOPNOTSUPP`) | HYPOTHESIS, **contradicted**: `flock` works and excludes a second holder (`errno=35`) | `selftest.json` of 1.2.8; the rule stays as a robustness test |
| `chmod` | `chmod`/`fchmodat` unavailable (`ENOSYS`); `fchmod` works | HYPOTHESIS, **contradicted**: `chmod`, `fchmod`, `fchmodat` all succeed | same |
| `fsync` | `fsync`/`fdatasync` rejected (`EINVAL`) | HYPOTHESIS, **contradicted**: `fsync` succeeds | same |
| (not a rule) Default **thread stack is 64 KiB** (`bytes=65536`); the main thread's limit is 2 MiB | functions with frames above ~48 KiB must not run on a worker thread | OBSERVED | `selftest.json` of 1.2.8; `tools/check_stack_frames.py` lists the frames |
| (not a rule) Creating a folder in `/data/dkrr` fails with `EACCES` | profile folders are writable, the data root is not | OBSERVED | `selftest.json` of 1.2.8 |

`DKRR_PS5_ENV=all` enables every rule. `tools/test_ps5_storage.py` runs the
shipped PS5 storage code (and the PS5 `InstallLock`/`durable_write` sources)
under `all`; `tools/test_ps5_env_shim.py` checks that each rule changes exactly
what it claims.

## Adding a fact

1. Observe it on the console (journal, FTP listing, screenshot) and note the file and time here.
2. Add a rule to `tests/ps5_env_shim.c`, a line to `tests/ps5_env_probe.cpp` and an entry in `tools/test_ps5_env_shim.py`.
3. Run the affected tests with `DKRR_PS5_ENV=all`. A new failure is the bug this was meant to catch.

Never add a rule for something that was not observed without marking it HYPOTHESIS.

## Reading evidence from a console

```
python3 tools/pull_console_debug.py <console-ip>      # read-only FTP, newest 3 journals + logs
```

It writes `debug-pulls/<time>/REPORT.txt`: build id, last step before the end,
and, if `tools/archive_symbols.py` archived that build, the crash location as
function and line. Builds before 1.2.8 write their journals with mode 600; the
tool then says so instead of guessing.
