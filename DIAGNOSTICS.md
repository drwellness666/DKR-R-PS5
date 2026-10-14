# Matched session, crash and frame-time diagnostics

The game keeps a small own-session journal under `/data/dkrr/diagnostics` and
shows its input fingerprint in **Graphics → Renderer & performance**. The private
candidate manifest pairs this build ID with SHA256 of the final eboot and its
matching private symbol ELF. The fingerprint includes patched source and game
archives; the final ELF hash is authoritative for address resolution.

## Crash recording

Initialization runs after the public SDK environment is ready and before C++
constructors. It uses the pinned PS5 SDK's `ps5platform/context.h`; its compile
assertion requires the verified PS5 ucontext layout. No guessed register offsets
or raw kernel/process structures are used. Existing nondefault framework signal
handlers are preserved. Optional signal imports are checked before use.

For SIGSEGV/BUS/ILL/FPE/ABRT, a fixed buffer records signal/code, fault address,
RIP/RSP, trap/error, runtime entry address and last startup/presentation stage.
The handler performs one best-effort write to an already open own journal and
queues the same signal under the restored default disposition. It does not
allocate, unwind, printf, lock, fsync, rewrite RIP or attempt recovery. The OS
remains responsible for termination. Signal delivery on an exhausted stack,
kernel/GPU failures that bypass user handlers, loader rejection before entry
and a complete power loss cannot be guaranteed by a user-space recorder.

Journals are exclusive ordinary files with bounded normal notes, not ROM/save
files. A bad/missing own directory disables journal creation without blocking
game startup. Logs may contain local paths or game state; keep raw logs and
symbol ELFs private and share only a reviewed summary. Files are not silently
deleted or uploaded. Tiny journals accumulate across sessions; cleanup is manual.

## Optional background ELF and PC capture

The public [klogsrv](https://github.com/ps5-payload-dev/klogsrv) transport exposes
`/dev/klog` over TCP 3232. Use an existing service when available. The optional
`build_session_relay.sh` builds a small equivalent read-only transport: it binds
before opening klog, refuses an occupied port without terminating another process,
accepts one PC client, buffers at most one 4096-byte chunk, and disconnects a
blocked client after two seconds. It uses ordinary socket/file calls, no process
enumeration, app.db edits, title launches or GPU/FPS hooks. Native compilation and
actual host-loopback transport tests pass; console streaming is pending.

The relay is an optional support payload, not another test game/build. Start it
only in the resumed authorized test session. A different klog reader can consume
the shared stream, so do not run multiple services; occupied-port refusal preserves
an already running logger. The PC collector does not launch/install anything:

```bash
python3 tools/collect_session.py
# Offline instructions only. When testing, explicitly capture:
python3 tools/collect_session.py --capture --host YOUR_PS5_LAN_IP \
  --output /ABSOLUTE/private/NEW_SESSION --build-id BUILD_ID --seconds 900
```

Capture flushes received chunks to the PC and preserves the raw OS/loader stream
even if the title exits. Duration is bounded (maximum one hour) and capture stops
at 64 MiB. Connection loss is labelled as connection loss, never itself a kernel
panic diagnosis. Only messages emitted before disconnection/power loss can be
saved. The structured game crash record lives in the game's journal; the relay
captures whatever the OS actually emits to klog, not arbitrary crash memory.

Copy only the own session journal through the existing normal file-management
route after a fault; the PC collector does not add a hidden file-download service.
Analyze it with the frozen private manifest and symbol file:

```bash
python3 tools/analyze_session.py /ABSOLUTE/private/run.log \
  /ABSOLUTE/private/CANDIDATE.json /ABSOLUTE/private/symbols.elf
```

The analyzer verifies the exact ELF SHA256, x86-64 ELF structure, executable
entry/segments and build ID. Load base comes from runtime entry minus that ELF's
entry; it is never guessed as 0x400000. Different-build or out-of-segment addresses
are retained as unattributed, without misleading source lines. Local addr2line
resolves eligible RIPs and inline locations only. Function names remain useful
where an older archive has no line tables; source-line availability is reported
by addr2line rather than invented. This candidate includes private line tables
for the changed UI, Vulkan adapter and authored native startup/adapters. Raw OS error lines are retained
separately; they are not assigned to this build without evidence. No external
service receives logs, game code or ROM-derived artifacts.

## FPS and microstutter measurements

The upstream FPS/performance overlay is already available. This candidate adds
**Start 60-second frame-time capture / Stop and save** in that existing Graphics
section. The hook runs only after a successful/suboptimal Vulkan present return,
outside the queue lock. Failed presents are not counted. Normal play does not
read the capture clock; while enabled, the hook updates lock-free counters with
no allocations, file writes, network sends or extra GPU waits.

The bounded result reports present-return rate, maximum gap and counts over
33.33/50/100 ms, plus dropped-sample count. Interval elapsed starts at the first
actual present. A live snapshot can lag across fields by one frame; the stopped
snapshot is used for analysis. No per-frame log is generated. This measures
successful presentation calls, not HDMI scanout, GPU completion or simulation
FPS; it does not require a kstuff FPS implementation. Time in menus is included.
Open the existing graphics disclosure again and press Stop and save to journal
the result after the automatic limit; automatic expiry itself does no disk I/O.

An optional upstream `power_profile=1` comparison records more detailed existing
regions, including pipeline creation, through the existing stderr logging path.
It is off by default. Use `summarize_performance.py` for those original traces;
use `analyze_session.py` for the new summary/journal. Neither host timings nor
an average FPS establishes that PS5 microstutters have been fixed.

## Local verification

`test_ps5_diagnostics.py`: actual C formatting/recording, 60s boundaries,
disabled capture, UI actions, missing/symlink directory refusal, repeated init,
existing-handler preservation and contained SIGSEGV/SIGABRT default termination.
`test_session_tools.py`: matching ELF/entry/segment checks, wrong-build/hash refusal,
chunked PC capture, actual relay transfer and occupied-port refusal over loopback.
`test_display_metadata_update.py`: qualified metadata restriction and actual combined
three-copy update/resume/repeat/restore, refusal before writes for foreign/symlink/temp
contents. Sanitizer tests use synthetic data, never a ROM or console connection.

SDK source: [qualified context header](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b83202be73e930050e00de0f1b4d9ec46c0391bf/platform/include/ps5platform/context.h).
