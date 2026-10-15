# Contributing to the PS5 integration

Use the source-only integration repository. A supported own ROM is a private
local build input; it is not needed for the default CI checks. Do not commit
ROMs, extracted assets, generated game CPU functions, private game binaries or
installers, save/Pak files, console dumps or third-party texture packs. Deleting
these in a subsequent commit does not remove them from Git history.

Keep platform changes in reviewed adapters or reproducible patches. Preserve
upstream copyright/licence notices. Follow [licence review](docs/LICENSES.md) and the upstream
asset policy. Name dependency commits and archive hashes; do not silently replace
a pinned source with a moving branch or prebuilt proprietary SDK artifact.

Explain the concrete trigger, before/after behavior, local checks and remaining
console acceptance. Visual correctness, physical rumble, persistent saves and
frame pacing require their own evidence. Compiler/present/transfer success is
not a substitute. Use [known issues](docs/KNOWN_ISSUES.md) and [diagnostics](docs/DIAGNOSTICS.md) for known cases.

Regenerate the selected CPU payload through the normal pipeline when a guest
policy changes; never hand-edit generated game C. Source patches must apply to
the pinned pristine input with their exact pre/post hashes and no fuzz.

New artwork or fonts need documented source, licence and attribution. Title artwork has separate provenance notes; Microsoft Selawik stays under OFL.
Do not add character art, music or logos extracted from a game to this repo.

Use the bug/build report templates and only short relevant diagnostics. Remove
private addresses and unrelated system information before sharing a log.
