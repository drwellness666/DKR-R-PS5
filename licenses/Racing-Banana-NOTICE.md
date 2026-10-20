# Racing Banana interface font

The PS5 interface uses the unmodified Racing Banana supplied in DKR-R 1.0.8,
commit 34ed57ae97ef46759e5b23aa8d7a1e7ec89f3f56. Upstream identifies its source
SHA-256 as AEADA6E5FF1388D27CC5D29DE9C67B74C0F6D851D6C772EE35376AF4E288C896.
It is an independent interface font, not extracted from the game ROM.

The project maintainer confirmed permission on 18 October 2026 for use and
redistribution in DKR-R PS5, including its installer and updates. This is the
maintainer's confirmation, not a general standalone-font licence or GPL
relicensing. The original permission correspondence is not reproduced here.
[Recorded scope and hashes](CONTENT-PERMISSIONS.json).

The original DKR-R font loader is used unchanged: Racing Banana for menu text
and signs, Jumpman for headings and digits, Selawik for reading text, and the
original default/monospace fallback policy. No extra Selawik glyph merge is
added to Racing Banana. The font data, glyph ranges, sizes and metrics match
the pinned upstream source.

Selawik's SIL OFL 1.1 and Jumpman's original EULA remain included. Jumpman's EULA
expressly permits software embedding and distinguishes it from standalone font
downloads.

Source: https://github.com/ThatGuyMcd/DKR-R/blob/34ed57ae97ef46759e5b23aa8d7a1e7ec89f3f56/THIRD_PARTY.md#fonts
