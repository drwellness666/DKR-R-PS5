# PS5 title presentation: paired selection and launch images

The accepted original DKR-R icon is retained. The unchanged upstream README
sky/wordmark image is used for **both** the selected-title background and the
launch transition, following the latest user choice. No retouched character art
or extracted ROM model is used. Icon and both DDS bytes match the earlier paired
artwork. The game hides the launch overlay after its first completed presentation,
using the already user-accepted native handoff.

| File | Role | Format |
| --- | --- | --- |
| `sce_sys/icon0.png` | Home-screen tile | 512px PNG |
| `sce_sys/pic0.dds` | Selected-title background | 3840×2160 single-surface BC7 DX10 DDS |
| `sce_sys/pic1.dds` | Launch transition, same image | Identical to pic0 |

The current app has 34 files. The native template recommends both background
files or neither. The converter's shorthand produces a matching pair, and the
port validates dimensions, format, exact surface size and equality. Source
provenance is in `TITLE_ART_SOURCES.json` and `homebrew/DKR-R-Game/ARTWORK.md`.
Build validation does not establish visibility in a cached Shell registration.

Primary references:
[presentation assets](https://github.com/blackbearreloaded/ps5-native-app-boilerplate/blob/main/docs/PRESENTATION_ASSETS.md),
[platform notes](https://github.com/blackbearreloaded/ps5-native-app-boilerplate/blob/main/docs/PLATFORM_NOTES.md),
[ShadowMount install source](https://github.com/drakmor/ShadowMountPlus/blob/1.7/src/sm_install.c),
[ShadowMount API](https://github.com/drakmor/ShadowMountPlus/blob/1.7/docs/api.md).

## Existing-title updates

The initial and registered-art installers carry icon/pic0/pic1. The combined
private updater requires the same recorded identity and matching paired art at
all three own locations: `/data/homebrew/PPSA94678/sce_sys`,
`/user/app/PPSA94678/sce_sys` and `/user/appmeta/PPSA94678`. It preflights all app
and metadata guards before changing code/notices and keeps this pair untouched.
The exact predecessor rollback preserves the same art. Neither operation
changes Shell registration, ROMs, profiles or saved data.

The earlier selection-only retirement/restore helpers remain as reviewed source
for diagnosing old known states. They are **not part of the current paired-art
update** and should not be sent for this beta. Old private snapshots remain
available for comparison; no old poster is silently deleted or restored.

See [UPDATE_GUIDE.md](UPDATE_GUIDE.md) for explicit local source/build/update
steps and completion markers. Helpers refuse unknown files and collisions.
Per-file replacement is atomic; the whole bundle is not one transaction.

## Why the missing home-menu background remains open

The icon and both DDS files were already verified in all three own locations,
yet the user reported no selected-title background. Another identical copy or
format check cannot establish the missing Shell state. Reviewed ShadowMount
1.7 source skips metadata staging on an ordinary remount when appmeta exists,
and skips registration when the title is already in app.db. A normal rescan is
therefore not proof of a metadata refresh. The installed version is unverified;
this source finding is a possible explanation, not a confirmed console diagnosis.

`tools/audit_title_metadata.py` reads a private **local** database copy in
read-only mode. It selects only PPSA94678's title/metadata/icon/pic0/pic1 fields,
never changes the database and excludes unrelated data. Its fixture passes.
No actual console database has been read in this offline work.

In the later console session, identify the installed loader/version and inspect
the own registration fields and correlated log before choosing its documented
own-title refresh route. Do not patch app.db blindly, clear global caches,
guess IPC signatures or rerun the quarantined status probe. A supported own-title
reinstall may be needed for cached metadata; that has not been performed here.

## Physical acceptance

1. The accepted icon remains visible.
2. Selecting the title shows the upstream sky/wordmark background.
3. Launch shows the matching image and releases it at the first completed frame.
4. Picture/audio, Touchpad UI, normal return and second launch pass.
5. Existing profiles and normal save/Pak reload remain intact.

Items 2–5 need the actual console; a local compile or file update is not their
acceptance evidence.
