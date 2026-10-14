# Console features and current limits

## Game language: German with a supported US dump

A PAL engine is not required to access the German text already present in the
US game. The pinned decomp's `src/menu.c`, `set_language`, explicitly documents
that German remains intact even though the US options screen hides it.
`load_menu_text` selects the German menu data, and `src/game.c`, `level_name`,
selects the German names. No translated game data is shipped in this repository.

The native US build extends **the original game's Options → Language** control
for both US 1.0 and US 1.1. Move left/right to cycle English, French and German;
left reverses the sequence. The checked CPU pipeline changes only the language
argument at the two original setter calls. `set_language` still loads the game's
own text and marks settings for its normal EEPROM save lifecycle. No translation
assets, PAL engine or direct host save writes are added.

The exact loaded images of both US ELFs, their function bounds and original
instructions are checked before generation. A native build refuses a stale CPU
pipeline without this adapter. Host tests execute the actual generated Options
loop, getter and setter for both revisions, including neutral input, unchanged
other settings, original text/save calls and register preservation. A separate
real upstream EEPROM codec test checks checksum and progress/record preservation.
These tests do not prove that every German scene renders correctly on a PS5 or
that settings survive hardware save/reload; those acceptance checks remain open.

The upstream Save Manager remains an alternative: before a game starts, open
**Save Manager → Offline → Edit progress → Unlocks → Game language** and apply
English/German/French through its normal confirmation and backup flow. In-game
editing is locked. Japanese is not offered because US 1.0 does not contain its
complete text. The Recompiled settings interface itself remains English.

## ROM revisions

One supported own US dump is enough to compile: v1.0/v77 or Rev A/v1.1/v80. An
optional build with both own dumps includes both payloads. Region and revision
come from a full content checksum, not the filename. In the dual build, open the
Touchpad pause page and select **Play → Game version → US 1.0 / US 1.1 – Rev A**.
The preference is written atomically under `/data/dkrr/preferences`, a dedicated
writeable folder provisioned by the private installer. The old root preference
is read only when no new choice exists. ROM/save folder permissions and names
are preserved. The preference takes effect on the next native title launch. Close normally and
start again; the current session never changes revision or saves. A single-ROM
build uses its included revision. Unavailable choices are disabled.

The established PS5 folders `/data/dkrr/profile-us-v77` and
`/data/dkrr/profile-us-v80` remain unchanged. Their full names were selected by
the adapter, while v77/v80 revision IDs come from upstream. The Save Manager
shows **Diddy Kong Racing — US 1.0 / US 1.1** and discloses the technical path
under **Storage location**. This is a label change, with no save migration.

EU/PAL engines are not in
the current port. PAL support would require independently matching its CPU
payload, revision addresses/hooks, ROM identity, video cadence and runtime data;
renaming a PAL file or allowing its hash alone does not provide that support.

## Controller Pak and Rumble Pak

Upstream provides four virtual 32-KiB Controller Pak images, separate from
Adventure EEPROM progress. In **Controls → Device → Shared controls / Controller
Paks** (the disclosure is labelled **Shared settings**), enable **Memory Pak** and **Rumble Pak** as desired. Both may remain enabled:
the virtual storage adapter answers storage requests and the rumble routing
continues to serve vibration requests. The Save Manager offers Pak transfer and
backup. No physical N64 accessory is required.

The existing **Identify with vibration** button on the Controls device page sends
a short 450-ms vibration pulse. The PS5 SDL backend translates normal SDL rumble
to `scePadSetVibration`; trigger-rumble/adaptive-trigger effects are not implemented.
Rumble strength is available in the Modern profile. Actual DualSense vibration
and a game-created Pak file surviving exit/restart still need PS5 tests. Host Pak
policy/storage tests do not prove those physical outcomes.

## Title presentation

The native game uses DKR-R's actual launcher icon and the unmodified header from
its pinned README. The converter scales/encodes them as a 512x512 PNG icon and
a 3840x2160 BC7 DDS selection background. Custom launch artwork is excluded. It does not repaint figures,
balloons, hats or logos. The output is under `$DKR_GAME_WORK/private/title-art`;
`TITLE_ART_SOURCES.json` records original hashes, source paths and credits.

The source repository includes only the verified recipe/provenance. Its build
obtains these public upstream inputs locally; no ROM models or textures are
needed for the title graphics. Artist and upstream terms remain applicable;
these pictures are not placed under the port's old generic-art CC0 notice.
The generated drafts were rejected and are excluded from the current export.

Local format validation passes. On 13 October the registered copies under
`/user/app/PPSA94678/sce_sys` and `/user/appmeta/PPSA94678` were updated and
verified. The user now confirms the original icon is visible. The background
remains invisible, despite correct file formats and copies; shell acceptance is
still open for pic0; pic1 is now intentionally absent in the candidate. The full app title stays in `param.json`.
The optional `tools/build_native_title_art_installers.py` builds exact-recorded
art-only payloads for these own metadata directories. It does not update the
application database or force a shell refresh.

The requested home-menu-only presentation, exact old-poster retirement and
registration investigation are documented in [TITLE_PRESENTATION.md](TITLE_PRESENTATION.md).
