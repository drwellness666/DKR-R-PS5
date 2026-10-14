/*
 * ps5-native-app-boilerplate - Native application startup.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * DKR-R integration: allocation-free startup breadcrumbs for native diagnosis.
 *
 * Sets the IEEE floating-point state, initializes the platform runtime, runs
 * static constructors and main, and hands normal process termination back to
 * the platform runtime.
 */

#include <cstddef>
#include <cstdint>

using Destructor = void (*)();
using Initializer = void (*)();

extern "C"
{
    int sceKernelDebugOutText(int, const char *);
    void _init_env(void *process_parameters);
    void ps5_fp_ieee(void);
    int atexit(Destructor callback);
    [[noreturn]] void exit(int status);
    int application_main(int argc, char **argv, char **envp) __asm__("main");

    extern Initializer __preinit_array_start[] __attribute__((weak));
    extern Initializer __preinit_array_end[] __attribute__((weak));
    extern Initializer __init_array_start[] __attribute__((weak));
    extern Initializer __init_array_end[] __attribute__((weak));
    extern Initializer __fini_array_start[] __attribute__((weak));
    extern Initializer __fini_array_end[] __attribute__((weak));
}

namespace
{
void run_forward(Initializer *first, Initializer *last) noexcept
{
    if (first == nullptr || last == nullptr)
        return;
    while (first != last) {
        // Avoid allocation and formatted I/O while constructors are running.
        char line[]="[DKR-R] startup: initializer=0x0000000000000000\n";
        const auto address=reinterpret_cast<std::uintptr_t>(*first);
        constexpr char hex[]="0123456789abcdef";
        for (unsigned i=0;i<16;++i)line[31+i]=hex[(address>>((15-i)*4))&15];
        sceKernelDebugOutText(0,line);
        (*first++)();
    }
}

void run_reverse(Initializer *first, Initializer *last) noexcept
{
    if (first == nullptr || last == nullptr)
        return;
    while (last != first)
        (*--last)();
}
} // namespace

extern "C" __attribute__((weak)) void catchReturnFromMain(int status)
{
    (void)status;
}

extern "C" void _init()
{
    run_forward(__preinit_array_start, __preinit_array_end);
    run_forward(__init_array_start, __init_array_end);
}

extern "C" void _fini()
{
    run_reverse(__fini_array_start, __fini_array_end);
}

extern "C" [[noreturn]] __attribute__((visibility("default"))) void
_start(void *process_parameters, Destructor loader_teardown)
{
    sceKernelDebugOutText(0,"[DKR-R] startup: entry\n");
    const int argc = *static_cast<const int *>(process_parameters);
    auto *parameters = static_cast<std::uint8_t *>(process_parameters);
    auto **argv = reinterpret_cast<char **>(parameters + sizeof(std::uint64_t));

    /* The console starts a title with flush-to-zero and denormals-are-zero
     * on; the code linked here expects the IEEE state (PS5 platform, fp.h). */
    ps5_fp_ieee();
    sceKernelDebugOutText(0,"[DKR-R] startup: initialize environment\n");
    _init_env(process_parameters);
    sceKernelDebugOutText(0,"[DKR-R] startup: environment initialized\n");
    if (loader_teardown != nullptr)
        (void)atexit(loader_teardown);
    (void)atexit(_fini);
    sceKernelDebugOutText(0,"[DKR-R] startup: constructors begin\n");
    _init();
    sceKernelDebugOutText(0,"[DKR-R] startup: constructors complete; main\n");
    const int status = application_main(argc, argv, nullptr);
    catchReturnFromMain(status);
    exit(status);
}
