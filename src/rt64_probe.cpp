// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic N64 RDP/VI test through the actual RT64 renderer. No game or ROM.
#include "hle/rt64_application.h"
#include "gbi/rt64_display_list.h"
#include <ps5platform/fp.h>
#include <ps5platform/klog.h>
#include <ps5platform/heap.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

extern "C" int sceSystemServiceHideSplashScreen(void);
static void report(const char *format,...) {
    char text[1024],line[1200];va_list args;va_start(args,format);
    std::vsnprintf(text,sizeof(text),format,args);va_end(args);
    std::snprintf(line,sizeof(line),"[DKR-R] rt64-probe: %s\n",text);
    sceKernelDebugOutText(0,line);
}
static void interrupt() {}
#include "ps5_native_close.h"
alignas(16) static uint8_t rdram[8*1024*1024],dmem[4096],imem[4096],header[64];
static uint32_t registers[32];
static int run() {
    report("build=rt64-native-v9 synthetic RDP/VI (no ROM)");
    ps5_fp_ieee();setenv("MESA_SHADER_CACHE_DISABLE","true",1);
    RT64::Application::Core core={};
    core.HEADER=header;core.RDRAM=rdram;core.DMEM=dmem;core.IMEM=imem;
    uint32_t **pointers[]={&core.MI_INTR_REG,&core.DPC_START_REG,&core.DPC_END_REG,&core.DPC_CURRENT_REG,
        &core.DPC_STATUS_REG,&core.DPC_CLOCK_REG,&core.DPC_BUFBUSY_REG,&core.DPC_PIPEBUSY_REG,&core.DPC_TMEM_REG,
        &core.VI_STATUS_REG,&core.VI_ORIGIN_REG,&core.VI_WIDTH_REG,&core.VI_INTR_REG,&core.VI_V_CURRENT_LINE_REG,
        &core.VI_TIMING_REG,&core.VI_V_SYNC_REG,&core.VI_H_SYNC_REG,&core.VI_LEAP_REG,&core.VI_H_START_REG,
        &core.VI_V_START_REG,&core.VI_V_BURST_REG,&core.VI_X_SCALE_REG,&core.VI_Y_SCALE_REG};
    for(size_t i=0;i<sizeof(pointers)/sizeof(pointers[0]);++i)*pointers[i]=&registers[i];
    core.checkInterrupts=interrupt;
    *core.VI_STATUS_REG=0x00003203;*core.VI_ORIGIN_REG=0x00100000;*core.VI_WIDTH_REG=320;
    *core.VI_V_SYNC_REG=525;*core.VI_H_SYNC_REG=3093;*core.VI_H_START_REG=(108u<<16)|748u;
    *core.VI_V_START_REG=(34u<<16)|514u;*core.VI_X_SCALE_REG=512;*core.VI_Y_SCALE_REG=1024;
    RT64::ApplicationConfiguration config;
    config.detectDataPath=false;config.useConfigurationFile=false;config.dataPath.clear();
    auto app=std::make_unique<RT64::Application>(core,config);
    app->threadsAvailable=2; // Bounded worker count while testing new pipelines.
    app->userConfig.graphicsAPI=RT64::UserConfiguration::GraphicsAPI::Vulkan;
    app->userConfig.internalColorFormat=RT64::UserConfiguration::InternalColorFormat::Standard;
    app->userConfig.resolutionMultiplier=1.0;
    app->userConfig.idleWorkActive=false;
    report("RT64 setup starting (shader library, workers, swapchain)");
    auto result=app->setup(0);
    report("RT64 setup result=%d",int(result));
    if(result!=RT64::Application::SetupResult::Success) { app->end();return 1; }
    report("RT64 setup=PASS device=%s",app->device->getDescription().name.c_str());
    auto *commands=reinterpret_cast<RT64::DisplayList *>(rdram+0x1000);
    // Own primitive: 320x240 RGBA32 colour image, fill cycle, full scissors,
    // cyan fill colour, rectangle and full sync. All words are host-u32, as
    // RT64's raw RDP parser expects; this is not cartridge-derived content.
    const uint32_t words[][2]={{0xff18013f,0x00100000},{0xef300000,0x00000000},
        {0xed000000,(1280u<<12)|960u},{0xf7000000,0x00ffffff},
        {0xf6000000|(1276u<<12)|956u,0x00000000},{0xe9000000,0x00000000}};
    for(size_t i=0;i<sizeof(words)/sizeof(words[0]);++i) { commands[i].w0=words[i][0];commands[i].w1=words[i][1]; }
    for(unsigned frame=0;frame<120;++frame) {
        app->processDisplayLists(rdram,0x1000,0x1000+sizeof(words),false);
        app->updateScreen();
        if(frame==0) { report("first synthetic RDP/VI frame queued; TV result needs observation");
            report("hide-splash rc=0x%08x",unsigned(sceSystemServiceHideSplashScreen())); }
        usleep(100000);
    }
    report("120 synthetic RDP/VI frames queued; shutting down RT64 workers");
    app->end();app.reset();
    report("RT64 cleanup=PASS (queued frames alone do not prove pixel correctness)");
    return 0;
}
int main() {
    // Route library errors to the existing public platform klog bridge.
    ps5_klog_capture_stderr("[DKR-R library] ");
    int status;
    try { status=run(); }
    catch(const std::exception &e) { report("exception: %s",e.what());status=2; }
    struct ps5_heap_stats stats={};ps5_heap_stats(&stats);
    report("completed status=%d heap-peak=%zu heap-libc-fallbacks=%llu",status,stats.peak_bytes,stats.libc_fallbacks);
    report("requesting system-service closure of this verified title only");
    // Allow the diagnostic reader to record completion before requesting closure.
    usleep(200000);
    report("self-close returned rc=0x%08x",unsigned(dkrr_close_this_title()));
    report("waiting for system-service closure; PS5 menu can close the title if refused");
    for(;;)usleep(100000);
}
