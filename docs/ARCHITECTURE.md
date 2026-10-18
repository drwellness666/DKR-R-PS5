# Installation architecture

| Lifetime | Location / rule |
|---|---|
| Replaceable program | One native dual-US app under `/data/homebrew/PPSA94678` |
| Owned ROMs | `/data/dkrr/roms`; verified and imported once, never embedded in a shared installer |
| Settings and textures | Existing common US 1.0 profile; shared by both revisions |
| Progress and Paks | Existing separate revision profiles; never installer targets |
| Install records and recovery | `/data/dkrr/installation`, outside scanned homebrew folders |

The consumer interface is a prebuilt installer ELF and a prebuilt update ELF.
Both contain the same native program. Users supply both US ROMs through USB;
no compiler, PC build receipt or source checkout is needed on the console.
The PC source build remains a developer option.

The installer searches `DKRR` and the root of USB drives 0–7. It identifies ROM
contents, supports z64/n64/v64 byte orders and generates 32 private controller
cues per revision locally. Optional controller features remain in the game's Controls → PS5
page and default off. A short installer uses PS5 notifications and the receiver
log; it creates no graphical launcher or background service.

Each managed program includes a bounded file manifest and a monotonic release
sequence. Updates verify the installed files on the console. The first public
baseline is a fresh 1.0 installation; private previews are not supported upgrade
targets. Unknown or modified installations are preserved. ROMs, progress, Paks
and settings are outside program replacement. A shared game lease prevents an
installer from replacing a cooperating running version.

The new app is prepared in a release-specific staging directory, outside
ShadowMountPlus scan paths. Exact known partial staging writes can resume.
Verified old versions are retained for recovery. A durable transaction record
covers the two directory renames. Both parent directories are synchronized after
each move before the recovery record is cleared; this is not an atomic rename exchange. Rerun
the payload after an interruption to restore or finish before launching. Unknown
files stop the transaction. No automatic deletion of old recovery versions is
performed; internal storage usage therefore grows with retained versions.

Stable native title metadata avoids unqualified shell-cache migrations. An
existing registered title with different metadata is refused before app writes. The
installer uses ShadowMountPlus' documented folder discovery and adds exactly its own source
path to the watched manual.lst, preserving existing entries and comments. It does not edit app.db, patch the kernel, introduce HTTP API
requirements or install extra save services. The manual entry also covers custom scan roots.

Console menus retain upstream page IDs while deriving visual and shoulder-button
order from one shared table. A single gamepad section selector replaces horizontal
subsection rows; Graphics separates display controls from HUD/detail controls.
Sections compiled out of the native build are omitted.

Controller audio is loaded on the speaker worker and cached per revision. Its
bounded 32-cue cache also retains the missile mix; disabling output still
closes the native speaker port. Extra sounds observe actual original audio
calls through linker wrappers for both revisions. Collectible scopes preserve
registers and identify the local racer; menu/loading/pause/rollback gates discard
unwanted events. A per-player latest-event mailbox expires old events; lower
priority cues never interrupt an active or pending item cue. Identical program assets share an embedded blob
inside the payload; installation still writes and verifies every target file.

Rendering, output rate and original game simulation remain separate. No global
water/interpolation change or forced shader-cache policy is used to conceal
unexplained stutters. Tracks-preview water and sustained frame pacing still
need measured console reproduction. Optional HDR10 is integrated; broader loader/display qualification remains open.

These are implemented candidate mechanisms, not physical-console acceptance.
Shared binaries require the matching source/licence package and a component
review. The planned 1.0 downloads are ROM-free installer/update ELFs with matching source.
[Validation](VALIDATION.md).

## Further native features

The game checks a bounded, Ed25519-signed release manifest over native system
HTTPS. The manifest binds the title, monotonic installer sequence, exact payload
size, SHA-256 and release tag; download addresses are derived locally. Only this
repository's release path and GitHub's HTTPS asset host are allowed. The download
is streamed to an unlinked, process-owned temporary inode, then checked in full.
The new client downloads **DKRR-Update.dkru**, a bounded data-only package.
It never calls the ELF receiver or executes a downloaded installer. The one-time
universal installer grants uid 1 directory-entry replacement in this title's
verified app tree only. No parent homebrew/system folder is made writable.

Preparation verifies the installed manifest, every public package file and
retained private controller sounds. The title metadata and libc runtime must
remain byte-identical. Runtime changes require a separately qualified manual
upgrade; a title-metadata change is currently refused by both update paths. The worker
prepares complete new files and verified old copies in a process-owned 0700
folder. The game then follows its existing save/shutdown path and joins its I/O,
controller and update workers. The same installation lease is upgraded to an
exclusive lock before activation.

Changed program files are replaced atomically. Unchanged metadata and the loaded
libc remain in place. The app folder keeps its identity for ShadowMountPlus'
existing mount. eboot is replaced
last among program files, followed by the complete installation manifest. A
fsynced journal and old file copies allow the native entry point to restore an
interrupted transaction before starting gameplay. Copies are retained throughout
recovery so interruption of recovery is repeatable. A committed update is
verified and its temporary recovery copy retired on the next launch.

The journal directory ancestry is synchronized before activation. A verified
complete transaction is retired durably before cleanup deletes its journals or
backups. An incomplete activation marker is abandoned only when the complete old
app still matches. Metadata errors never mean missing files. The manual
installer refuses a pending native transaction rather than racing or overwriting
it. Unknown files, symlinks, hardlinks, mismatched files and downgrades are refused.

Checks, downloads, verification and staging run on a joined worker. Checks at
launch are optional, default off and shared by both ROM revisions. There is no
resident service, child installer process, kernel backend or PC agent. The 1.0
client only reads `updates/native-channel.txt` and never contacts port 9021.
The release builder does not produce the old private-preview network ELF.

Private GitHub releases cannot be fetched anonymously. No token is embedded;
the public channel becomes reachable only when the owner opens the repository.
The manual ELF route remains available. The signing key is kept outside source;
`tools/sign_update_channel.py` refuses a key that does not match the client's
public key. Newer upstream DKR-R versions still need reviewed port integration.

The optional HDR10 path retains SDR scene/UI rendering, composes separate
brightness layers and converts to 10-bit BT.2020/PQ for native VideoOut. SDR
allocates no HDR layers. Calibration snapshots are coherent and lock-free;
preference I/O cannot block the present worker. Display size/rate and HDR share
one versioned `tv-output.txt` transaction. Valid legacy files are imported on
first save and preserved; an invalid current record never resurrects old choices.
Output and revision writes reserve fresh temporary entries, preserve interrupted/
unknown ones, atomically replace the validated target and synchronize its parent.
The native update card estimates space for download, stage, recovery and rollback;
a measured shortage is refused before downloading. Unknown telemetry is explicit.

The first public installation includes the final HDR title declaration. Private
preview migration is outside the release scope. Physical HDR brightness, SDR
fallback and coexistence with VRR/120 Hz remain console checks.
See [Display](DISPLAY.md) and the [pre-release review](PRE_RELEASE_REVIEW.md).
The [architecture review](CONCEPTUAL_REVIEW.md) records the fresh-install baseline and update
contract decisions.
