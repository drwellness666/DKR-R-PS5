# Recompiled-project comparison — 18 October 2026

The first public release uses a fresh installation. This review compares useful
runtime techniques without changing game rules or the pinned renderer wholesale.

| Technique | Primary implementation checked | DKR-R PS5 decision |
|---|---|---|
| Reuse conversion scratch instead of allocating every audio block | [BanjoRecomp audio](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/src/main/main.cpp), [Snap64Recomp audio](https://github.com/JackandBeans/Snap64Recomp/blob/cf53b89272b9db19059dd97eed2690ea1dd94844/src/audio.cpp) | Existing buffers were already reused. Additionally reserve the maximum accepted scratch size while opening the paused device, before delivery. Both normal conversion and custom-music scratch then avoid vector growth for accepted blocks. |
| Apply saved graphics choices before renderer setup | [Zelda64Recomp renderer](https://github.com/Zelda64Recomp/Zelda64Recomp/blob/b65c482ed672258eb684ab94a4e8c8aa662615ee/src/main/rt64_render_context.cpp), [Snap64Recomp renderer](https://github.com/JackandBeans/Snap64Recomp/blob/cf53b89272b9db19059dd97eed2690ea1dd94844/src/rt64_render_context.cpp) | Already present: DKR-R applies the effective graphics configuration before RT64 setup. Retain this ordering, including the PS5 Vulkan selection. |
| Warm a persistent list of observed shaders | [Snap64Recomp manual](https://github.com/JackandBeans/Snap64Recomp/blob/cf53b89272b9db19059dd97eed2690ea1dd94844/docs/MANUAL.md) | Potential later performance work. Its renderer revision and seen-shader mechanism differ from our pinned RT64. A list from another game is not a valid DKR workload. A useful implementation requires DKR shader keys, renderer integration, bounded compatible persistence and measured cold/warm runs. Do not enable the unqualified PS5 disk cache by default. |
| Adjust an audio queue to hide device stalls | Same Banjo/Snap audio sources | Their host resampling and queue-feedback assumptions differ. DKR-R already separates its emulated AI DMA state from its bounded host cushion. Keep that model; importing another game's timing formula could change pacing or pitch. |
| Retain native-resolution framebuffer work for correctness | [RT64 renderer design](https://github.com/rt64/rt64#features) | Do not disable readbacks or native framebuffer work globally to conceal preview water or frame-time spikes. Isolate those faults on the same DKR scene and settings. |

The scratch check compiles the actual setup/resizing operations with a counted
allocator, checks varied block sizes and rate changes, and preserves zeroed
custom-music input. The existing exact neutral-mixer comparison remains valid.
This establishes less allocation in those two vectors, not zero allocation in
SDL/decoders or a measured PS5 FPS gain. At up to 48 kHz, their combined reserved
sample capacity is 576,000 bytes before allocator overhead.

No shaders, game-specific patches or game data from these projects are bundled.
The Tracks-preview flash and reported microstutters remain known issues until a
reproduced cause and verified fix exist. [Test status](VALIDATION.md).
