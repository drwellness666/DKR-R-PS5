# Updating

Updates to this port are distributed as source. A GitHub ZIP is not a ready-to-run
PS5 game. Build the new version locally with your own ROM, then apply a compatible
update. ROMs and save folders do not need to be copied again when unchanged.
An upstream DKR-R release needs an updated port recipe; changing the pinned
revision manually is not supported.

## Check for a source release

```bash
python3 tools/update_source_release.py --channel beta
python3 tools/update_source_release.py --channel beta \
  --download "$PWD/work/private/new-source-beta"
```

The first command only checks. The second verifies GitHub's asset digest and
extracts into a new directory; it does not run downloaded code or contact a PS5.
While the repository is private, unauthenticated downloads will not work. Use
GitHub's signed-in release download. Never share credentials in an issue.

## Preserve your previous local build

Keep the exact previous application, its `CANDIDATE.json` and symbols. These are
private recovery inputs. Do not overwrite them with a new build. The first-install
assistant refuses a different existing application; it is not a force-reinstall
button. Save data is described separately in [Saves](SAVES.md).

## Prepare and apply a compatible update

This advanced route requires a clean source directory, the supported ROM
variables from [Build](BUILD.md), the previous app and its recorded manifest hash:

```bash
bash tools/build_local_update.sh \
  /ABSOLUTE/private/previous/PPSA94678 \
  "$PWD/work/private/new-update" \
  RECORDED_PREVIOUS_MANIFEST_SHA256
```

It builds locally and prepares `private/update-pair` under that new work directory.
It sends nothing. The format requires a known complete predecessor and compatible
title metadata/artwork. Unknown layouts or conflicting output declarations stop
the build instead of overwriting a different app.

```bash
python3 tools/run_preview_update.py /ABSOLUTE/private/update-pair
python3 tools/run_preview_update.py /ABSOLUTE/private/update-pair \
  --host YOUR_PS5_LAN_IP --apply
```

The first command checks locally. Close DKR-R before `--apply`. Success requires
the updater's completion marker, not merely a completed transfer. Do not retry
blindly after an interrupted operation. `--rollback` selects the paired previous
application; ROMs, configurations, saves and Paks are outside this update.

Files are replaced individually, so this is not a single atomic transaction.
Offline tests cover refusal, partial resume and rollback. The complete public
source-download/rebuild/console-update path still needs wider testing. There is
no in-game automatic updater in this beta.
