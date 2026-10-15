# PS5 GPU build details

For the normal game build, follow README.md or README.de.md. This document
explains the GPU dependency; no separate probe installation is required.

## Toolchain and pinned inputs

`VULKAN_SOURCES.json` pins the PS5 Vulkan/RADV, Mesa, public SDK source and host
build dependencies by revision and hash. The PS5-native GPU build uses the
project's qualified public SDK basis; SDL/transfer payloads use SDK v0.44.
These are separate build inputs. No licensed Sony SDK or copied Sony module is
included. Native imports resolve the console's existing libraries at runtime.

The checked Linux host uses Clang/LLD 23.1.1 and CMake 3.31.6. Development
requirements include Python with venv/pip, Make, pkg-config, PCRE2, glslang,
LLVM/libclc development files, LLVM-SPIRV and SPIR-V Tools. `build_gpu.sh` creates
its Meson/Ninja Python environment locally and installs no system packages.

```bash
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
```

Use a dedicated absolute work directory. Downloads are checked before use;
modified or incomplete prepared sources are refused. `DKR_GPU_JOBS` limits
parallel compilation; `CMAKE_BIN` selects an installed CMake when required.
The build contacts public source servers, not the PS5.

## Integration

The game statically links the native RADV path. Plume selects a compatible
Vulkan display/plane and creates a native VideoOut surface. Desktop SDL/X11
Vulkan-window creation is not used. Import checks reject unresolved desktop
symbols. The driver source patch in `patches/0006-ps5-driver-policy.patch`
disables periodic diagnostic writes by default and retains the bounded refresh
cadence guard. It does not prove a general performance improvement.

Normal output is SDR/FIFO. Resolution and nominal refresh requests, measured
fallback and the difference between output buffers and game rendering are
explained in DISPLAY_FEATURES.md. Current console evidence is in WORK_STATUS.md.

The optional GPU probe recipes remain developer diagnostics. Their native title
uses a different ID from the game. A successful probe is not game acceptance;
do not install it as an extra step in the normal user workflow.
