# Racing Banana interface font

The PS5 interface uses the unmodified Racing Banana supplied in DKR-R 1.0.6,
commit c3eda4d368afbc729ed4a71312cce38d27f87c37. Upstream identifies its source
SHA-256 as AEADA6E5FF1388D27CC5D29DE9C67B74C0F6D851D6C772EE35376AF4E288C896.
It is an independent interface font, not extracted from the game ROM.

Upstream says that public distributors must confirm redistribution permission.
No separate general redistribution grant was found in the pinned source or its
current third-party notice. This notice records provenance and is not itself
a licence grant. The font is not relicensed under GPL by this port. The source
repository includes the integration recipe, not a standalone font download.

Selawik Regular/Semibold provide missing characters under SIL OFL 1.1; their
licence is retained separately. Jumpman is not linked into this PS5 target.
Its supplied EULA expressly permits software embedding and distinguishes it
from standalone font downloads; its removal was not based on a blanket ban.

Source: https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/THIRD_PARTY.md#fonts
