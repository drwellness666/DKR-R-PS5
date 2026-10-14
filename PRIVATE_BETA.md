# Private source beta 1

This is an experimental **source beta**, based on pinned DKR-R 1.0.6. The repository
starts private. Access for other testers is arranged separately by the repository
owner. No game binary or ROM-bearing update is provided through GitHub.

## What testers can build

The main Linux recipe in [GAME_BUILD.md](GAME_BUILD.md) builds both US 1.0 and
US 1.1 in one native application when both own dumps are supplied. A single
supported dump can also be used. In a dual build, Touchpad → Play → Game version
selects the revision for the **next launch**. Existing profiles remain separate.
The original Options → Language control offers English, French and German in
both revisions. The Recompiled interface remains English; PAL engines are deferred.

Use [README.md](README.md) for prerequisites and first-install commands. Build
outputs under `work/` stay private. The initial installers refuse an existing
installation with different bytes; an installer is not a general overwrite tool.
The title ID `PPSA94678` is provisional. Resolve a foreign-title collision before
installation, without overwriting that title.

## What is already evidenced

An earlier US 1.0 build was actually played on the reported PS5 setup. Picture,
audio, controls, HUD, save-slot names and Play/Online text were user-confirmed.
The current dual/language/home-menu candidate compiles and links natively and
passes its FSELF integrity check. Actual revision selection and ImGui navigation
pass local tests. Six actual Linux game boots cover both revisions and all three
languages, using NullRenderer and fresh private EEPROM fixtures. These boots do
not prove rendered language text, audible output or PS5 save persistence.

The current candidate has **not** been installed or accepted on the console.
A source-only CI pass is not a PS5 compatibility or gameplay certificate.

## Hardware qualification before wider use

Use [BETA_TESTING.md](BETA_TESTING.md) and record PASS, FAIL or NOT TESTED separately:

1. Start the existing baseline through the normal loader before changing files.
   The most recent session rejected both a new dual build and the restored,
   previously playable single build before a fresh game log. Its cause is open.
2. Test both revisions, switching only for the next launch and preserving profiles.
3. Check German/French text and language persistence after a normal exit/relaunch.
4. Check the new Play page, controller focus, save/Pak write and actual reload,
   physical vibration and its stop on exit.
5. Check the selected-title background and matching launch image, followed by
   its prompt release at the first completed game frame. The icon was confirmed; the background was still absent.
6. Reproduce the short Tracks-preview water flash and occasional microstutters.
   Neither has a measured fix. Check Adventure intro with a fresh test save;
   existing progress is not reset to force the sequence.

Only one console/loader combination has prior gameplay evidence: firmware 13.60,
Relapse, WebKit Autoloader 0.6.1, kstuff-lite and ShadowMount-Plus. Exact versions of
the latter two are unverified. This is not broad firmware support.

## Exact-predecessor private updates

`tools/build_preview_update_set.py` can prepare an update and rollback **locally**
from two own complete app snapshots. It verifies the recorded predecessor manifest,
unchanged title identity/icon/selection art and the known former launch image.
It requires the expected registered metadata in all three own title directories;
missing or unknown files stop the operation. See [TITLE_PRESENTATION.md](TITLE_PRESENTATION.md).

The compiled helpers contain private game code and are never release assets.
`tools/run_preview_update.py ABSOLUTE_PRIVATE_PACKAGE` checks both ELF hashes and
sizes without making a connection. Sending requires an explicit `--apply` or
`--rollback` and `--host YOUR_PS5_LAN_IP`. Close the own title first. The sender
waits for the correct helper completion marker; a successful transfer alone is
not enough. On a stop, timeout or missing marker, keep the app closed and inspect
the result before any retry. There is no automatic resend or game launch.

The combined helper preflights code, title identity, known paired artwork state
and temporary-file collisions before changing files. Update and rollback are
repeatable on the permitted states. Replacement is atomic **per file**, not
for the complete bundle. Power loss and real console execution remain untested.
A rollback restores the predecessor files and preserves the identical paired artwork;
it does not alter ROMs, preferences, saves or Paks.

## Report and distribution rules

Report the source commit, ROM revision/hash, console/loader versions and the
smallest reproduction. Use the issue template with a short filtered own log.
Exclude ROMs, generated CPU code, game executables/installers, profiles/Paks,
texture packs, credentials, private addresses and full console dumps.

[LICENSE_REVIEW.md](LICENSE_REVIEW.md), [THIRD_PARTY.md](THIRD_PARTY.md) and original
notices describe the source licences and artwork limitations. ROM-free source
publication is not a licence to distribute the original game or third-party packs.
This beta does not make a legal guarantee about every user's local use.

The current beta adds an exact neutral-audio fast path and an opt-in guarded
RADV shader-cache comparison. Performance changes need a matched physical A/B
test before any stutter-reduction claim. See [UPDATE_GUIDE.md](UPDATE_GUIDE.md).

## Integrated candidate, 2026-10-14

The current test track combines both US revisions/languages, controller/menu changes, optional output and diagnostics in one app. See [INTEGRATED_TEST.md](INTEGRATED_TEST.md). Local native integrity and host tests are separate from pending hardware acceptance. [DIAGNOSTICS.md](DIAGNOSTICS.md) records the exact private ELF attribution policy and optional PC capture. The combined updater permits only an explicitly requested attribute3 0 -> 0x40040 transition with all other fields preserved and all three own metadata copies preflighted. It does not refresh shell registration or modify app.db. Prior separate candidates remain historical archives, not a routine rollback sequence.
