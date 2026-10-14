# EUR/PAL revisions and European languages

PAL integration is deferred by the project decision of 13 October 2026. This
retains the earlier source audit, not an active implementation target or a claim
of working PAL support. The deliverable is a dual US 1.0/1.1 native build with
English, French and German directly in the original Options menu. Do not enable PAL
by adding a checksum to the US build's allowlist.

## Verified public source foundation

The pinned Diddy-Kong-Racing decompilation revision is
`06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6` in `GAME_SOURCES.json`.
Its README supports these commands and its verification files give these hashes:

| Player version | Decomp identity | Canonical SHA-1 | Native port status |
| --- | --- | --- | --- |
| US 1.0 | us.v77 | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` | Native build and earlier picture/audio acceptance |
| US 1.1 / Rev A | us.v80 | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` | Local native build; gameplay acceptance pending |
| EUR 1.0 | pal.v77 | `dd5d64dd140cb7aa28404fa35abdcaba33c29260` | Identifiable; PAL payload/runtime not implemented |
| EUR 1.1 | pal.v80 | `b7f628073237b3d211d40406aa0884ff8fdd70d5` | Identifiable; PAL payload/runtime not implemented |

PAL's internal revisions are v77/v80, the same revision numbers as US but a
different region. Future selection/profile/pipeline keys must include **both
region and revision**. A numeric 77/80 choice alone cannot distinguish all four.

Sources: [decompilation README](https://github.com/DavidSM64/Diddy-Kong-Racing/blob/06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6/README.md),
[PAL 1.0 checksum](https://github.com/DavidSM64/Diddy-Kong-Racing/blob/06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6/ver/verification/dkr.pal.v77.sha1),
[PAL 1.1 checksum](https://github.com/DavidSM64/Diddy-Kong-Racing/blob/06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6/ver/verification/dkr.pal.v80.sha1).

## Languages already present

The PAL options code selects English, French and German. The same pinned
`src/menu.c` documents that German is fully intact in the US game; the US options
screen simply hides it. The native US adapter now extends the original Options
cycle for both US revisions. Actual generated getter/setter/menu code and the
upstream EEPROM codec pass local checks. Visible translated game text and
language persistence still need console acceptance. The Recompiled interface
stays English. See [console features](CONSOLE_FEATURES.md).

This does not add invented Spanish/Italian game text. Japanese is a separate
release and is explicitly incomplete in US 1.0; it is outside this request.
See [language source](https://github.com/DavidSM64/Diddy-Kong-Racing/blob/06cf09bab2e0a5f210d2a14a25cdf22c3c5b18c6/src/menu.c)
and [console features](CONSOLE_FEATURES.md).

## Address audit completed without game data

Run after preparing the pinned source tree:

```bash
python3 tools/audit_pal_addresses.py "$DKR_GAME_WORK" > pal-address-audit.json
```

The tool checks the public decomp archive hash, compares all four prepared symbol
files to that archive, then requires one symbol to match **both** existing US
runtime addresses and to exist in **both** PAL symbol maps. The current pinned
source has **102 of 102 runtime address fields** with unambiguous PAL candidates.
No ROM is read. It does not install address tables or enable PAL execution.

## Deferred work that would be required for a native PAL build

1. Validate the owner's EUR dumps against the region-specific full hashes and
   use distinct canonical inputs `dkr.pal.v77.z64` and `dkr.pal.v80.z64`.
2. Rebuild each matching decomp with `REGION=pal VERSION=v77` or `v80`; require
   its reconstructed full-ROM hash to match before generating CPU payloads.
3. Make the authored CPU patch pipeline region-aware. Its current revision
   switches, build paths and emitted wrappers target US. Verify every required
   hook/call signature and generated output against the matching PAL ELF.
4. Register PAL payloads and immutable region/revision identities in the runtime;
   verify the candidate address tables against those exact ELFs. Review hardcoded
   patch/menu/graphics/audio addresses beyond the shared 102-field address table.
5. Add separate PAL save, Pak, settings, ROM-cache and selection routes. Do not
   migrate or relabel the user's existing US progress as PAL progress.
6. Validate PAL's simulation/VI/audio cadence and presentation interpolation.
   A 60-Hz display must not speed up a PAL engine. Test both revision-specific
   gameplay behavior and English/French/German rendering.

The owner's matching EUR 1.0 and 1.1 decompilation builds were locally verified
against their full ROM hashes. Their private outputs are preserved, but native
PAL CPU generation, integration and console tests are deferred. Until those
steps pass, the build continues to reject PAL rather than boot it through US
addresses. `tools/check_rom.py` can identify known EUR dumps read-only while
keeping `supported=false`. ROMs, extracted assets, generated CPU functions and
game binaries remain private and outside the public repository.
