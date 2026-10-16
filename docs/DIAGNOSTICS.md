# Reporting bugs and measuring performance

Start with the [known issues](KNOWN_ISSUES.md). A useful report names the port
version, active US revision, firmware/loader, graphics settings and exact steps.
A short relevant DKR-R log excerpt is sufficient. Keep ROMs, saves, game builds,
private addresses and complete console logs out of GitHub reports.

## Frame-time capture

Open **Graphics → Renderer & performance**. Use **Start 60-second frame-time
capture**, return to the scene you want to measure, then use **Stop and save**.
The result reports successful presentation-return rate, largest gap and counts
over timing thresholds. Time spent in menus is included.

This measures presentation calls, not HDMI scanout or simulation FPS. A 120-Hz
output mode does not prove 120 rendered FPS, and a clean minute does not cover
all tracks or texture packs. The existing performance overlay remains available.

## Game journal

The game writes bounded per-session journals to `/data/dkrr/diagnostics` when the
installation has provisioned that directory. A missing journal does not block
the game. Journals remain local and are not automatically deleted or uploaded.
The build ID shown in Graphics identifies the corresponding private build receipt.

The recorder can capture some user-space crashes. It cannot guarantee a record
for a kernel panic, GPU hang, loader rejection before entry or sudden power loss.
A disconnected logger is not by itself evidence of a kernel panic.

## Developer tools

Keep `$DKR_GAME_WORK/private/CANDIDATE.json` together with the exact application
and `$DKR_GAME_WORK/native-link/llvm-pie.elf`. The normal build assistant creates
the receipt. For a manual build:

```bash
python3 tools/record_native_build.py "$DKR_GAME_WORK"
```

For optional PC log capture, first use an existing klog service or build the
project's relay with `tools/build_session_relay.sh SDK_ROOT`. Do not run competing
klog readers. The relay uses TCP 3232 and refuses an occupied port.

```bash
python3 tools/collect_session.py --capture --host YOUR_PS5_LAN_IP \
  --output /ABSOLUTE/private/NEW_SESSION --build-id BUILD_ID --seconds 900
python3 tools/analyze_session.py /ABSOLUTE/private/run.log \
  /ABSOLUTE/private/CANDIDATE.json /ABSOLUTE/private/symbols.elf
```

Capture is bounded and does not launch or install the game. Analysis verifies the
matching symbol file before attributing addresses; it does not guess a load base.
`report_diagnostic_session.py SESSION_DIRECTORY CANDIDATE_JSON SYMBOL_ELF` combines
saved data locally. No cloud service is involved.

If the payload/file manager cannot read a mode-0600 journal, do not make all saves
or logs world-readable. `build_journal_reader.py SDK CANDIDATE_JSON RUN_BASENAME
NEW_PRIVATE_ELF` creates a reader for that exact journal/build. Send it explicitly
with `send_elf.py` during a console session and retain its marked output locally.
See tool `--help`/usage for the remaining developer options.
