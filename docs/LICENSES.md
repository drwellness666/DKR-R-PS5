# Licences and content

## Code

The PS5 adapters are **GPL-3.0-or-later**. DKR-R's original project code is MIT;
its combined runtime includes GPL components. Exact source revisions, notices
and build recipes are supplied with the port. [Component inventory](../THIRD_PARTY.md).

Universal ELF downloads must accompany the matching integration source and
recipes for the pinned dependencies. A root LICENSE does not replace component
notices or grant rights to the original game. Public homebrew SDK sources are
used; licensed Sony SDK files and Sony system modules are not distributed.

## Game data

Users supply both legally usable US ROM dumps locally. Shared packages exclude
ROMs, extracted controller audio, saves, Paks and third-party texture packs.
Generated CPU source trees and private ROM-containing installers stay off GitHub.
The universal installer creates its private controller audio on the console.

## Fonts and artwork

The interface uses DKR-R's original, unchanged font setup: **Racing Banana**
for menu text and signs, **Jumpman** for headings and digits, and **Selawik** for
reading text, including upstream's existing fallback behavior. Font data,
loading code, glyph ranges, sizes and metrics are kept identical.

Selawik's SIL OFL 1.1 and copyright remain included. Jumpman's supplied EULA
expressly permits software embedding; its original terms are included in the
component notices. Racing Banana is not extracted from the ROM. Upstream asks
public distributors to confirm redistribution rights; that confirmation is
not documented here yet. [Font notice](../licenses/Racing-Banana-NOTICE.md).

Title graphics and interface images come from the pinned DKR-R project. Its
application icon is credited to **POOTERMAN**. These images keep their original
terms and attribution; this port does not relicense them or the original game's
characters as its own work. The README links the upstream header directly.
[Artwork provenance](../homebrew/DKR-R-Game/ARTWORK.md).

Texture packs must be obtained separately under their authors' terms. Nintendo,
Rare, Microsoft and Sony do not endorse this project.

Primary notices: [DKR-R runtime licence](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/runtime-recomp/COPYING-NOTICE.md) ·
[asset policy](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md) ·
[third-party notices](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/THIRD_PARTY.md).

Original component texts: [notices](../licenses/COMPONENT-NOTICES.txt) · [source inventory](../licenses/COMPONENT-NOTICES.json).
