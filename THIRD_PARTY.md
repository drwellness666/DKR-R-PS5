# Third-party components and notices

This inventory covers the PS5 integration and its build dependencies. Source
manifests pin exact revisions and hashes. Original licence/copyright files stay
in the fetched or bundled source trees; additional notices are in `licenses/`
and copied into the private application. [Distribution scope](docs/LICENSES.md).

## Runtime and build components

| Component | Revision / inventory | Terms |
|---|---|---|
| DKR-R | 1.0.6, `c3eda4d368afbc729ed4a71312cce38d27f87c37`; `GAME_SOURCES.json` | Original project source MIT; combined-runtime obligations in upstream COPYING-NOTICE |
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

The PS5 UI uses unmodified Microsoft Selawik Regular/Semibold under SIL OFL 1.1.
The licence is `licenses/DKR-R-Selawik-OFL.txt`. Racing Banana and Jumpman are not
linked by this PS5 target. Upstream UI assets retain their separate terms.

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
their source components. This source package includes no ROM, generated game
payload, playable binary, save or third-party texture pack.
