# Graphics build notes

For normal use, follow the [build guide](BUILD.md) or run `python3 setup.py`.
The graphics build is a dependency of the game; no separate probe installation
is required.

`VULKAN_SOURCES.json` pins the public PS5 Vulkan/RADV, Mesa, SDK and host tools.
The game links RADV statically and uses a native VideoOut surface through Plume.
It does not create a desktop SDL/X11 Vulkan window. Console libraries are imported
at runtime; Sony SDKs and system modules are not distributed.

```bash
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
```

Use an absolute work path. `DKR_GPU_JOBS` limits parallel compilation and
`CMAKE_BIN` can select an installed CMake. Prepared sources and downloads are
checked against their manifests. Build environments are local; scripts do not
install OS packages or contact the PS5.

The driver policy disables recurring diagnostic writes during normal play.
That alone is not evidence that all stutters are fixed. Output modes, VRR and
render scale are explained in [Display](DISPLAY.md).

Current source tests and hardware results are in [Validation](VALIDATION.md).
Synthetic GPU probes are developer tools and do not establish game correctness.
