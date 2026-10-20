# Credits ❤️

**DKR-R PS5 exists because of the original DKR-R team and the open-source recompilation and PS5 homebrew communities.** Please visit and support their projects. Their authorship, copyright notices and licences remain with them.

## The game and recompilation

| Project / people | Contribution |
|---|---|
| **[ThatGuyMcd and all DKR-R contributors](https://github.com/ThatGuyMcd/DKR-R)** | The original Diddy Kong Racing Recompiled project, its interface and game features. This PS5 port is based on 1.0.7. |
| **[DavidSM64 and the Diddy Kong Racing decompilation contributors](https://github.com/DavidSM64/Diddy-Kong-Racing)** | Matching source, symbols and game structures. |
| **[Wiseguy and the N64Recomp contributors](https://github.com/N64Recomp/N64Recomp)** | Static recompilation, with [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) providing the runtime. |
| **[RT64 contributors](https://github.com/rt64/rt64)** | Rendering and presentation; [Plume](https://github.com/rt64/Plume) supplies graphics abstraction. |
| [Wave Race 64 Recomp](https://github.com/elliotttate/wave-race-64-recomp) | Enhanced-water reference used by upstream DKR-R. |
| [Golden Balloon](https://github.com/akratch/goldenballoon) | HUD-layout reference credited by upstream DKR-R. |

[Original DKR-R credits](https://github.com/ThatGuyMcd/DKR-R/blob/0368f51d769af4d89671a212e3ea8ac8078ce41b/README.md#credits-and-licensing) · [Original third-party acknowledgements](https://github.com/ThatGuyMcd/DKR-R/blob/0368f51d769af4d89671a212e3ea8ac8078ce41b/THIRD_PARTY.md)

## PS5 foundations

| Project / people | Contribution |
|---|---|
| **[John Törnblom and ps5-payload-dev](https://github.com/ps5-payload-dev/sdk)** | Public homebrew SDK and payload ecosystem, including PS5 SDL support. |
| **[Mihawk-99](https://github.com/mihawk-99/PS5_Vulkan)** | PS5 Vulkan integration, [PS5 Mesa/RADV](https://github.com/mihawk-99/PS5_Mesa) and [platform SDK](https://github.com/mihawk-99/PS5_PayloadSDK). |
| **[BlackBearReloaded](https://github.com/blackbearreloaded/ps5-native-app-boilerplate)** | Native app tooling, runtime and networking examples; [ps5-opengl](https://github.com/blackbearreloaded/ps5-opengl) graphics foundations and [ProsperoLight](https://github.com/blackbearreloaded/ProsperoLight) output-format reference. |
| [Mesa contributors](https://www.mesa3d.org/), [LLVM contributors](https://llvm.org/) and [Khronos](https://github.com/KhronosGroup/Vulkan-Headers) | Graphics driver, compiler and Vulkan foundations. |
| [drakmor / ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) | Homebrew discovery and registration used by this installation route. |
| [Kodi PS5 contributors](https://github.com/VivaLaVent/kodi-ps5) | Public output-status reference. |

Thanks also to the loader, jailbreak and homebrew communities that make native PS5 applications possible. These projects remain independently maintained.

## Artwork, fonts and sound

- **[POOTERMAN](https://www.deviantart.com/pooterman)** created DKR-R's application icon. The cloud/header artwork is the original image linked by DKR-R. [Artwork sources and terms](homebrew/DKR-R-Game/ARTWORK.md).
- **Racing Banana** is supplied by DKR-R. **Jumpman** is by **[Neale Davidson / Pixel Sagas](https://www.pixelsagas.com/)**; **[Selawik](https://github.com/microsoft/Selawik)** is by Microsoft. The port retains the original typography. [Font notices](licenses/Racing-Banana-NOTICE.md).
- **sr.gu** leads the optional DKR REMASTERED texture-pack project. Downloads are separate through the [DKR-R community](https://discord.com/invite/AMWfXdBjNP).
- **[Dietrich Epp / VADPCM](https://github.com/depp/vadpcm)** provides the decoder adapted for locally prepared controller sounds.
- **Rare and the original Diddy Kong Racing team** created the game. Game content belongs to its respective rights holders and is not licensed by this port.

## Supporting libraries and tools

The source also uses or builds on the work of these projects and their contributors:

[SDL](https://github.com/libsdl-org/SDL) · [Dear ImGui](https://github.com/ocornut/imgui) · [ImPlot](https://github.com/epezent/implot) · [libsamplerate](https://github.com/libsndfile/libsamplerate) · [GekkoNet](https://github.com/HeatXD/GekkoNet) · [Monocypher](https://github.com/LoupVaillant/Monocypher) · [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) · [ELFIO](https://github.com/serge1/ELFIO) · [fmt](https://github.com/fmtlib/fmt) · [Rabbitizer](https://github.com/Decompollaborate/rabbitizer) · [SLJIT](https://github.com/zherczeg/sljit) · [toml++](https://github.com/marzer/tomlplusplus) · [miniz](https://github.com/richgel999/miniz) · [o1heap](https://github.com/N64Recomp/o1heap) · [xxHash](https://github.com/Cyan4973/xxHash) · [XZ](https://github.com/tukaani-project/xz) · [zlib](https://zlib.net/) · [volk](https://github.com/zeux/volk) · [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) · [re-spirv](https://github.com/rt64/re-spirv) · [stb](https://github.com/nothings/stb) · [Zstd](https://github.com/facebook/zstd) · [ddspp](https://github.com/redorav/ddspp) · [hlslpp](https://github.com/redorav/hlslpp) · [im3d](https://github.com/john-chapman/im3d) · [Mupen64Plus](https://github.com/mupen64plus/mupen64plus-core) · [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) · [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) · [RT64 DXC builds](https://github.com/rt64/dxc-bin) · [IDO static recomp](https://github.com/decompals/ido-static-recomp) · [GNU binutils](https://www.gnu.org/software/binutils/) · [Flips](https://github.com/Alcaro/Flips) · [asm-differ](https://github.com/simonlindholm/asm-differ) · [asm-processor](https://github.com/simonlindholm/asm-processor) · [m2c](https://github.com/matt-kempster/m2c) · [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended).

This is an overview, not a replacement for the **[complete original notices](licenses/COMPONENT-NOTICES.txt)** and **[component inventory](THIRD_PARTY.md)**. Those records include additional inherited and optional source components.

## This port

Maintained by **[drwellness666](https://github.com/drwellness666)**. The PS5 integration was developed with AI assistance (vibe coding). This does not attribute the original game, DKR-R or third-party work to AI. [Contributions](CONTRIBUTING.md) are welcome.

[Licence overview](docs/LICENSES.md) · [Back to the game](README.md)
