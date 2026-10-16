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
contents, supports z64/n64/v64 byte orders and generates the 18 private controller
cues locally. Optional controller features remain in the game's Controls → PS5
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

Rendering, output rate and original game simulation remain separate. No global
water/interpolation change or forced shader-cache policy is used to conceal
unexplained stutters. Tracks-preview water and sustained frame pacing still
need measured console reproduction. HDR and broader loader/display support
remain separate work.

These are implemented candidate mechanisms, not physical-console acceptance.
Shared binaries require the matching source/licence package and a component
review. The currently published beta remains source-only. [Validation](VALIDATION.md).
