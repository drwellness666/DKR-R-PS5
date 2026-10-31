# Third-party components and notices

This inventory covers the PS5 integration and its build dependencies. Source
manifests pin exact revisions and hashes. Original licence/copyright files stay
in the fetched or bundled source trees; additional notices are in `licenses/`
and copied into the private application. [Distribution scope](docs/LICENSES.md).

## Runtime and build components

| Component | Revision / inventory | Terms |
|---|---|---|
| DKR-R | 1.0.8, `34ed57ae97ef46759e5b23aa8d7a1e7ec89f3f56`; `GAME_SOURCES.json` | Original project source MIT; combined-runtime obligations in upstream COPYING-NOTICE |
| N64ModernRuntime | `ae1ffbb909d9f93c88c41830deb539f7feef5ed2` | GPL-3.0 |
| N64Recomp | `81213c1831fab2521a6a5459c67b63437d67e253` | MIT |
| PS5 payload SDK | v0.44, `f5d40f02770e9b0001dd9e18056eca70b3da8b40` | Predominantly GPL-3.0-or-later; BSD headers retain separate notices |
| PS5 SDL2 | `ee4c47dc0d617b3bc8f35108f9956baf228a1322` | zlib |
| libsamplerate | 0.2.2, `c96f5e3de9c4488f4e6c97f59f5245f22fda22f7` | BSD-2-Clause |
| LLVM libc++, libc++abi, libunwind | SDK build recipes; LLVM 18.1.8 basis | Apache-2.0 WITH LLVM-exception |
| Plume | `d890ac899e505fb30040e037a4037cdeca68f033` | MIT |
| RT64 and shader host tools | `RT64_SOURCES.json` | RT64 MIT; dependencies retain their notices |
| PS5 Vulkan tooling and public SDK fork | `VULKAN_SOURCES.json` | GPL-3.0-or-later platform/build code; included notices retained |
| Mesa/RADV/ACO | `VULKAN_SOURCES.json` | Predominantly MIT; per-file licences apply |
| Vulkan-Headers | `VULKAN_SOURCES.json` | Apache-2.0 |
| volk / VulkanMemoryAllocator | `VULKAN_SOURCES.json` | MIT |
| zlib 1.3.1 / 1.3.2 | Bundled source archives | zlib |

Game build inputs also include decomp source, ELFIO, fmt, Rabbitizer, SLJIT,
toml++, GekkoNet, Monocypher, Mbed TLS, miniz, o1heap, xxHash and xz. Renderer
inputs include ImGui/ImPlot (MIT), re-spirv and Zstd (BSD/GPL dual licensing).
Their revisions are recorded in the source manifests; retain their original
notices. GNU binutils is a GPL host build tool. IDO-static-recomp comes from its
public decompals release. DXC comes from the pinned public RT64 host-tools source.

## Adapted public PS5 code

The PS5 Vulkan support archive includes the public native emitter by
BlackBearReloaded and integration/RADV recipes by Mihawk-99. Their copyright and
GPL-3.0-or-later notices are retained. Sony system libraries are imports, not
redistributed contents; no licensed Sony SDK is included.

`src/ps5_game_crt.cpp` adapts the pinned `tooling/native/app_crt.cpp`,
Copyright 2026 BlackBearReloaded, GPL-3.0-or-later.

`src/ps5_getcwd.c` adapts the directory traversal from PS5_PayloadSDK,
commit `b5efad528e0ac1b8289f39d72a0e890ff68ff0a6`,
`platform/src/directory.c`, SHA-256
`b092b6a628b9a9e9852da6b0995d848e871a7d7f91602b4da7b1a7e9277deed9`.
Copyright 2026 Mihawk and GPL-3.0-or-later remain in that adapter.

The emulated-TLS preparation retains the public SDK's Apache/LLVM-exception
notice and checks its original hash before changing cleanup timing. Updater
hashing uses John Törnblom's public streaming SHA-1 implementation from
`PS5_PayloadSDK/crt/nid.c`, GPL-3.0-or-later; generated code retains its notice.

## Fonts and artwork

`tools/vadpcm_decode.py` and the separate native `installer/vadpcm.c` adapt Dietrich Epp's VADPCM decoder
(Copyright 2022) under MPL-2.0. Its notice and the full licence in
`licenses/VADPCM-MPL-2.0.txt` are retained. The decoders run locally on owned ROM input. The native installer generates
private item PCM on the console; ROMs and generated sounds are excluded from
shared installer packages.
Source: [VADPCM decode.c](https://github.com/depp/vadpcm/blob/main/codec/decode.c).

The port keeps DKR-R's original interface typography: Racing Banana for menu text and signs; Jumpman for headings and digits; Selawik Regular/Semibold for reading text (SIL OFL 1.1); and upstream's default/monospace fallbacks. Font data, loading policy, sizing and glyph ranges are unchanged. Racing Banana provenance and the maintainer-confirmed redistribution scope are recorded in `licenses/Racing-Banana-NOTICE.md`. Jumpman's original EULA permits software embedding and is included in the component notices.

The title recipe fetches the original DKR-R icon and README header. Sources,
hashes and attribution are in `TITLE_ART_SOURCES.json` and
[ARTWORK.md](homebrew/DKR-R-Game/ARTWORK.md). Upstream credits the icon to
POOTERMAN. The port does not claim CC0 or original-game rights for these images.
Third-party texture packs are separate user inputs and are not distributed here.

## Bundled archives

Patches are stored separately and applied to checked source work trees. The
N64ModernRuntime and RT64 bundles omit upstream contributor-guidance documents;
build source and original licence/copyright files remain unchanged. Recorded
archive hashes describe these bundles. All other retained notices remain in
their source components. The source archive excludes ROMs, generated CPU source, game binaries, saves
and third-party texture packs. Separate universal ELF release previews contain
compiled program code and upstream interface/title artwork; original controller
PCM is generated from user ROMs on the console. Matching source, notices and
pinned build recipes accompany those previews.

## Native Internet updates

The HTTPS service sequence in `src/ps5_internet_update.cpp` follows
[ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate/blob/44ae1dab529654eac795d2a7c4f5a0425102b8d7/examples/update-check/update_check.c),
Copyright 2026 BlackBearReloaded, GPL-3.0-or-later. The public system certificate
store remains authoritative; certificate and host verification are never disabled.
Signature checking uses the optional Ed25519 module of the already pinned
Monocypher dependency (BSD-2-Clause / CC0-1.0), and SHA-256 uses the already pinned
Mbed TLS dependency (Apache-2.0). Their full licence notices are retained in `licenses/Monocypher.md` and
`licenses/Mbed-TLS.md`, and copied into the installed app.
No additional proprietary SDK, network service library or private release key is
shipped. The new in-game updater installs a signed data package in its own process and never contacts an ELF receiver. The first public release uses this native route.

## HDR output references

The optional PS5 HDR10 scanout uses the public packed-format contract from
[ProsperoLight](https://github.com/blackbearreloaded/ProsperoLight/blob/b1b2c5d8cde17347419091b95c30fa95f22fedd0/include/ps5_videoout_formats.h),
GPL-3.0-or-later, by BlackBearReloaded. The public 48-byte output-status ABI is
documented by [Kodi PS5](https://github.com/VivaLaVent/kodi-ps5/blob/main/overlay/xbmc/platform/ps5/VideoOutInfo.cpp),
Team Kodi, GPL-2.0-or-later. Our shader, calibration and bounded WSI adaptation
are supplied as source; no Sony SDK implementation or system module is copied.
The original component licence/copyright texts copied from the pinned build
inputs are retained in `licenses/COMPONENT-NOTICES.txt`, with source hashes in
`licenses/COMPONENT-NOTICES.json`.

## Exact notice verification

The original component bundle contains 57 sections. It includes PS5 Mesa,
platform/public SDK and native-tooling terms, re-spirv and both zlib versions.
The source-wide C-family catalogues retain original comments, including optional
and unlinked files; their presence does not mean every listed licence applies
to the executable. Source members and byte offsets are recorded in
[PLATFORM-NOTICE-SOURCES.json](licenses/PLATFORM-NOTICE-SOURCES.json).

Build checks verify every section's length and SHA-256 and refuse stale installed
copies. [Content permission confirmation](licenses/CONTENT-PERMISSIONS.json)
records the maintainer's scope for Racing Banana and title graphics.

Native HTTPS uses [curl](https://curl.se/) with the existing Mbed TLS library.
The pinned [Mozilla CA certificate store](https://curl.se/docs/caextract.html) is
provided under MPL 2.0. Original notices are included in `licenses/COMPONENT-NOTICES.txt`;
source versions and hashes are recorded in `HTTPS_SOURCES.json`.
