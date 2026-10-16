# Distribution and licences

This repository distributes PS5 integration source, patches, build recipes,
pinned public source archives, tests and notices. Each user supplies a legally
usable supported ROM and builds privately. The repository and releases exclude
ROMs, extracted game data, generated game CPU code, playable binaries, saves,
Controller Paks and third-party texture packs.

## Code and dependencies

The port's adapters are GPL-3.0-or-later. DKR-R's original project code is MIT;
N64ModernRuntime and the public PS5 platform contain GPL components. Dependencies
retain their own licences and copyrights. The detailed inventory is
[THIRD_PARTY.md](../THIRD_PARTY.md), with additional notices under `licenses/` and
in the pinned sources. A root LICENSE does not replace these notices.

The build uses public homebrew SDK sources and imports libraries already on the
console. It does not distribute licensed Sony SDK files or Sony system modules.
A source-only release is not a blanket permission to redistribute a locally
compiled game executable or ROM installer.

## Fonts and title artwork

The PS5 interface uses Microsoft Selawik Regular/Semibold under SIL OFL 1.1.
The build retains its OFL and copyright notice. Racing Banana and Jumpman are
excluded from this PS5 target; this is a distribution choice, not evidence that
those fonts caused earlier rendering bugs or that all other uses are prohibited.

Title graphics are fetched from the pinned DKR-R project through the documented
recipe. The launcher icon is credited upstream to POOTERMAN. Artwork retains
upstream/artist terms; it is not relicensed as CC0 by this port. See
[artwork provenance](../homebrew/DKR-R-Game/ARTWORK.md).

Texture packs must be obtained separately under their authors' terms. The tested
Remastered pack is not included or licensed for redistribution by this project.
Nintendo, Rare, Microsoft and Sony do not endorse the port.

## Primary notices

- [DKR-R runtime notice](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/runtime-recomp/COPYING-NOTICE.md)
- [DKR-R asset policy](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md)
- [DKR-R third-party notices](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/THIRD_PARTY.md)
- [SIL Open Font License](https://openfontlicense.org/open-font-license-official-text/)

These notes describe the distributed files and recorded licence terms; they do
not grant rights to the original game or replace legal advice.
