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
sequence. Updates verify the installed files on the console. Known early builds
have explicit migration inventories; unknown or modified installations are
preserved. ROMs, progress, Paks and settings are outside the program replacement.
A shared game lease prevents an installer from replacing a cooperating running
version. Always close older releases before their first migration.

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
need measured console reproduction. HDR and broader loader/display support
remain separate work.

These are implemented candidate mechanisms, not physical-console acceptance.
Shared binaries require the matching source/licence package and a component
review. Beta 3 includes ROM-free installer/update ELFs with matching source.
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
remain byte-identical; ABI migrations use the manual installer. The worker
prepares complete new files and verified old copies in a process-owned 0700
folder. The game then follows its existing save/shutdown path and joins its I/O,
controller and update workers. The same installation lease is upgraded to an
exclusive lock before activation.

Each file is atomically renamed over its old directory entry; the app folder
itself remains in place for ShadowMountPlus' existing mount. eboot is replaced
last among program files, followed by the complete installation manifest. A
fsynced journal and old file copies allow the native entry point to restore an
interrupted transaction before starting gameplay. Copies are retained throughout
recovery so interruption of recovery is repeatable. A committed update is
verified and its temporary recovery copy retired on the next launch. The manual
installer refuses a pending native transaction rather than racing or overwriting
it. Unknown files, symlinks, hardlinks, mismatched files and downgrades are refused.

Checks, downloads, verification and staging run on a joined worker. Checks at
launch are optional, default off and shared by both ROM revisions. There is no
resident service, child installer process, kernel backend or PC agent. Legacy
Beta 3.3 clients keep their separate signed ELF channel for migration; new clients
only read `updates/native-channel.txt` and never contact port 9021.

Private GitHub releases cannot be fetched anonymously. No token is embedded;
the public channel becomes reachable only when the owner opens the repository.
The manual ELF route remains available. The signing key is kept outside source;
`tools/sign_update_channel.py` refuses a key that does not match the client's
public key. Newer upstream DKR-R versions still need reviewed port integration.

HDR needs a 10-bit presentation path, correct color transfer, UI brightness
and SDR fallback. This target currently presents 8-bit SDR. Public
[PS5 HDR presenter research](https://github.com/blackbearreloaded/ps5-hardware-video-decoding-research/blob/main/docs/implementation.md)
provides a possible VideoOut direction; integration into the
[PS5 Vulkan driver](https://github.com/mihawk-99/PS5_Vulkan) and RT64 is separate work.
