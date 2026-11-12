# Debugging a console build

Everything here is evidence-based: each step names the file that proves it.
Nothing contacts the console unless you run `pull_console_debug.py`, and that
tool only reads.

## What the title records on the console

| File | Content | Survives a crash |
|---|---|---|
| `/data/dkrr/diagnostics/run-*.log` | **crash-safe journal**: `DKRR_SESSION` (build id, entry address), `DKRR_STEP` (timestamped steps of storage, install lock, crash reports, chat, self-test), on a fault `DKRR_CRASH` (signal, fault address, RIP, RSP, stage) and `DKRR_STACK` (stack words pointing into the image). Written with `write()`, no queue. Mode 666 since 1.2.8. | yes |
| `<profile>/logs/selftest.json` | what the console's file system and libc did, measured at every start (see `PS5_FACTS.md`) | yes |
| `<profile>/logs/storage-trace.log` | steps of the last storage pass (also in the journal) | yes |
| `<profile>/logs/runtime.log` | the game's log through a bounded queue; the tail is lost in a crash | no |

## After any failure on the console

1. `python3 tools/pull_console_debug.py <console-ip>` (needs the console's FTP; read-only).
2. Read `debug-pulls/<time>/REPORT.txt`:
   - `ended: NO session end` and a `last step before the end` name the step.
   - With the matching `symbols/<build-id>/llvm-pie.elf` the report names the function and source line of RIP.
3. `selftest.json` shows whether the console refused something the code relies on.

The unstripped ELF of each build is archived by the release chain
(`tools/archive_symbols.py`) under `symbols/<build-id>/`. Without it a report
still shows steps and raw addresses, and says so.

## Functions the console does not export

The platform leaves an import it cannot resolve bound to address 0; the first
call dies with `DKRR_CRASH rip=0x0`. The journal then has `DKRR_NULLCALL`, and
`pull_console_debug.py` prints `UNRESOLVED IMPORT: <name>` and the calling
function. At every start the missing imports are journalled as steps
`sub=imports`; the tool flags any that `docs/IMPORTS_BASELINE.json` does not
know. The build runs `tools/check_import_baseline.py` on the linked ELF: it
fails if a stand-in from `src/ps5_missing_libc.c` is lost and warns about each
import that has not been checked on a console yet.

## Before a console test

| Check | Command |
|---|---|
| journal, handler, step markers | `tools/test_ps5_diagnostics.py` |
| a real fault becomes function:line | `tools/test_ps5_crash_report.py` |
| PS5 file-system behaviour on the PC | `tools/test_ps5_env_shim.py`, `tools/test_ps5_storage.py <work>` (runs under `DKRR_PS5_ENV=all`) |
| self-test | `tools/test_ps5_selftest.py` |
| large stack frames in the linked image | `tools/check_stack_frames.py <llvm-pie.elf> --baseline docs/STACK_BASELINE.json` |

## Fixed order for a new upstream DKR-R version

1. Rebase the native adaptations (`rebase_native.py`); every adaptation has a test, a failing one names the file.
2. `rebuild` and link. Run `check_stack_frames.py`: a **new** large frame is a possible worker-thread overflow; compare it with the stack size in the last `selftest.json`.
3. Run the PC tests above, including the PS5 environment run.
4. Package, run the real-client update tests, archive the symbols.
5. Send to the console once. Start the game, wait 30 s, then `pull_console_debug.py`: `selftest.json` and the journal tell whether anything differs from `PS5_FACTS.md`.
6. A new console behaviour becomes a new rule in `tests/ps5_env_shim.c` before the fix, so the PC reproduces it.
