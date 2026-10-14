# Source updates and private incremental title updates

The port is distributed as source. The native executable currently incorporates
locally generated game CPU code; this project does not upload that executable.
A GitHub source release therefore cannot be installed directly as a PS5 game.
Updates can still preserve the installation, ROMs, preferences and saved progress:
check/download the port release, build privately on Linux, then replace only
changed known application files. There is no need to upload an unchanged ROM again.

The release helper uses the port's release list, including prereleases on the beta
channel. It does not automatically follow a new **upstream DKR-R** tag: each upstream
version needs a reviewed port patch and pinned inputs. A changed upstream revision
must not be substituted blindly into the current hash-bound 1.0.6 recipe.

## Check or download a port release

```bash
python3 tools/update_source_release.py --channel beta
python3 tools/update_source_release.py --channel beta \
  --download "$PWD/work/private/new-source-beta"
```

The first command only checks release metadata; the second downloads/extracts a
new source directory. The expected asset is `DKR-R-PS5-source.zip`, with a
GitHub-provided SHA256 digest. Size/digest, member paths, duplicate names,
links/special entries and expansion limits are checked before extraction.
No downloaded script runs automatically and no console connection is made.
A missing release, private-repository access failure or missing digest does not
fall back to an unchecked download. For private access, a locally configured
`DKR_GITHUB_TOKEN` may be used; never put it in a command argument, repository or
report. It is sent only to api.github.com and stripped on cross-origin redirects.
Alternatively use GitHub's normal authenticated source download/clone workflow.

## Build a private update and rollback

Keep the exact preceding local application snapshot and its recorded manifest.
From a **clean new source export**, set the supported own ROM paths as documented
in README, then explicitly request the rebuild:

```bash
export DKR_ROM_V77="/absolute/path/to/your/own/US-1.0.z64"
export DKR_ROM_V80="/absolute/path/to/your/own/US-1.1.z64"
bash tools/build_local_update.sh \
  /absolute/private/previous/PPSA94678 \
  "$PWD/work/private/local-update-build" \
  RECORDED_PREVIOUS_MANIFEST_SHA256
```

This opt-in helper runs the four existing pinned build recipes in a new directory,
then prepares `private/update-pair/update.elf`, `rollback.elf` and `READY.json`.
It does not replace the old local build or send anything. The ROM paths are local
inputs; the build never uploads them. A full fresh toolchain build can take time.
Known prior SDK/GPU builds can instead be used with the documented separate
`build_preview_update_set.py` builder, when their pinned inputs still match.

This first combined update format requires no new app files, unchanged title
identity/icon/paired selection-launch artwork, the exact registered metadata and
a recorded complete predecessor. If a future release changes that structure,
the builder stops. Its reviewed release recipe must then provide an appropriate
migration; the helper does not force an unknown structure through an old updater.

## Apply only when ready to test

```bash
python3 tools/run_preview_update.py /absolute/private/update-pair
python3 tools/run_preview_update.py /absolute/private/update-pair \
  --host YOUR_PS5_LAN_IP --apply
```

Close DKR-R first. The initial command checks both payloads without a connection.
The send waits for the own updater's operation-specific completion marker.
A transfer-only response, timeout, STOP or wrong marker is not success; keep the
app closed and inspect the outcome before any resend. There is no automatic retry,
foreign-title termination, mount change, database patch or title launch.

An explicitly requested `--rollback` uses the paired predecessor payload, checks
known states and restores its application bytes. ROMs/profiles/Paks are not touched.
Changed files are replaced atomically one by one; the complete bundle is not one
transaction. Console execution and power-loss recovery need qualification.

## Current evidence and remaining automation

Actual C update/rollback and source-download/receipt logic pass offline fixtures,
including tamper, wrong identity, partial resume and refusal cases. The source
release downloader never connects to a console. The four build steps are the same
recipes already used for native builds; a complete future downloaded-release
rebuild and real console update have not yet been exercised end to end.

There is no in-game background installer in this beta. The prepared host workflow
avoids full title reinstallation for compatible updates while retaining a manual
apply boundary and exact rollback. A fully automatic on-console updater requires
an independently redistributable native component/layout and documented native
registration/lifecycle handling; it is later work, not advertised as finished here.

## Integrated candidate, 2026-10-14

The current test track combines both US revisions/languages, controller/menu changes, optional output and diagnostics in one app. See [INTEGRATED_TEST.md](INTEGRATED_TEST.md). Local native integrity and host tests are separate from pending hardware acceptance. [DIAGNOSTICS.md](DIAGNOSTICS.md) records the exact private ELF attribution policy and optional PC capture. The combined updater permits only an explicitly requested attribute3 0 -> 0x40040 transition with all other fields preserved and all three own metadata copies preflighted. It does not refresh shell registration or modify app.db. Prior separate candidates remain historical archives, not a routine rollback sequence.

## Preserve the installed output declaration before rebuilding

The local update helper now checks the exact predecessor manifest before creating the new build directory or starting toolchain work. It accepts only the own title and qualified attribute3 values 0 / 0x40040, and carries that declaration into the rebuild. An explicitly conflicting DKR_PS5_HIGH_REFRESH_BUILD is refused; upgrading the declaration still requires the separate qualified metadata transition. Private line tables are enabled for the rebuild. ROMs, saved preferences and profiles stay outside the application update. This prevents a later update of an integrated capable title from reverting its declaration to ordinary mode merely because a child shell's environment ended.


## Share an existing own pack between the US profiles

DKR-R keeps `profile-us-v77` (US 1.0) and `profile-us-v80` (US 1.1 / Rev A)
separate. Texture choices and controller bindings are intentionally per profile.
Keep that upstream-compatible structure. Build the small helper from your
existing upstream-imported, canonical RT64 ZIP; it embeds only its name, size
and hash. The SDK SHA1 input is the pinned public `PS5_PayloadSDK/crt/nid.c`:

```bash
python3 tools/build_texture_pack_copy.py   "$DKR_GPU_WORK/PS5_Vulkan/.deps/native/ps5-payload-sdk"   /ABSOLUTE/PRIVATE/srgu-remastered-v0.2.7.zip   "$DKR_GAME_WORK/private/copy-own-pack-to-us11.elf"   --sha-source "$DKR_GPU_WORK/PS5_PayloadSDK/crt/nid.c"
```

Start US 1.1 once to create its own profile, then close the title normally.
The source pack must already exist in
`/data/dkrr/profile-us-v77/texture-packs/` with that exact basename and identity.
Send the prepared helper through your existing ELF loader. Wait for
`DKRR PACK COPY PASS` before starting the same game again. Open **Textures**
in US 1.1 and enable that pack if it is not already selected. The copy consumes
one additional pack's worth of storage; no full texture upload is needed.

The helper drops only its own effective identity to the known game UID, validates
ordinary directories and the entire source hash, copies to a new temporary
file, syncs and reads it back before publishing. A repeat of the same copy
is harmless; different existing destinations, symlinks or old partial files
stop for inspection. It never edits controller settings, saves, Pak files,
ROMs, shell registration or another process. Keep the title closed to avoid
concurrent profile writes. Hash-checked own copies are private; this mechanism
does not grant distribution rights for someone else's texture artwork.

## Refined candidate, 2026-10-15

The preceding integrated build is physically accepted for both US revisions,
normal image/audio, original icon, home background and matched launch artwork.
The next offline candidate adds full-size 4K UI, normal-operation queue-log
suppression, bounded 120-Hz transition retries and qualified journal support.
An update from that exact accepted manifest changes the executable only;
registered metadata and paired artwork remain identical. A hash-backed
rollback is retained for deliberate emergency use. No automatic game restart,
rollback, console update or GitHub main/release replacement takes place.
