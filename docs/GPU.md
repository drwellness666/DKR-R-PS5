# PS5 graphics build

Part of the [full source build](BUILD.md); players do not need a graphics probe or separate driver installation.

```bash
export DKR_GPU_WORK="$PWD/work/gpu"
bash build_gpu.sh
```

Use an absolute work path. `DKR_GPU_JOBS` limits parallel jobs; `CMAKE_BIN` selects CMake when needed. `VULKAN_SOURCES.json` pins public driver, SDK and host-tool inputs.

The game links RADV statically and uses native VideoOut through Plume. Sony system libraries are imported at runtime; no licensed Sony SDK or system modules are bundled.

[RT64 build](RT64.md) · [Display settings](DISPLAY.md) · [Credits](../CREDITS.md)
