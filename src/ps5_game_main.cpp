// SPDX-License-Identifier: GPL-3.0-or-later
// Native title entry around the authored DKR-R runtime. ROM data stays external.
#include <ps5platform/fp.h>
#include <ps5platform/klog.h>
#include <ps5platform/heap.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <cerrno>
#include <fcntl.h>
#include <initializer_list>
#include <atomic>
#include <sys/stat.h>
#include <unistd.h>
#include "ps5_controller_mapping.h"
#include "ps5_render_diagnostics.h"
#include "ps5_shader_cache.h"
#include "ps5_revision_selection.h"
#include "ps5_display_options.h"

int dkrr_game_main(int, char **);
extern "C" unsigned dkrr_report_missing_imports(void (*)(const char *));
extern "C" int sceKernelOpen(const char *,int,int);
extern "C" int sceKernelClose(int);
extern "C" int sceKernelStat(const char *,struct stat *);
static std::atomic_bool runtime_error_reported=false;
extern "C" void original_message_box(const char *) __asm__("__real__ZN11ultramodern14error_handling11message_boxEPKc");
extern "C" void native_message_box(const char *) __asm__("__wrap__ZN11ultramodern14error_handling11message_boxEPKc");
extern "C" void native_message_box(const char *message) {
    runtime_error_reported.store(true,std::memory_order_relaxed);
    original_message_box(message);
}
extern "C" __attribute__((visibility("hidden"))) const char **getargv(void) {
    static const char *args[]={"/app0/eboot.bin",nullptr};return args;
}
static void report(const char *format, ...) {
    char text[1024], line[1200]; va_list args;va_start(args,format);
    std::vsnprintf(text,sizeof(text),format,args);va_end(args);
    std::snprintf(line,sizeof(line),"[DKR-R] native-game: %s\n",text);
    sceKernelDebugOutText(0,line);
}
static void missing_import(const char *name) {
    report("unresolved function import: %s",name);
}
#define DKR_NATIVE_TITLE_ID "PPSA94678"
#include "ps5_native_close.h"

static bool ordinary_rom(const char *path) {
    struct stat info={};
    return stat(path,&info)==0 && S_ISREG(info.st_mode) && info.st_size==12*1024*1024;
}
static void configure_render_diagnostics() {
    int fd=open("/data/dkrr/render-diagnostics.txt",O_RDONLY|O_NOFOLLOW);
    if(fd<0)return;
    struct stat info={};char text[513];size_t bytes=0;
    bool valid=fstat(fd,&info)==0 && S_ISREG(info.st_mode) && info.st_size>=0 && info.st_size<=512;
    while(valid && bytes<sizeof(text)) {
        ssize_t n=read(fd,text+bytes,sizeof(text)-bytes);
        if(n<0 && errno==EINTR)continue;
        if(n<0){valid=false;break;}if(!n)break;bytes+=size_t(n);
    }
    close(fd);unsigned flags=0;
    if(!valid || !dkrr_parse_render_diagnostics(text,bytes,&flags)) {
        report("render diagnostics rejected: require exact known 0/1 keys, regular file <=512 bytes");return;
    }
    for(unsigned key=0;key<DKR_RENDER_DIAGNOSTIC_COUNT;++key) {
        // Some upstream reports test presence, rather than the value. Remove
        // disabled flags so an explicit zero really disables those reports.
        if(flags&(1U<<key))setenv(dkrr_render_environment[key],"1",1);
        else unsetenv(dkrr_render_environment[key]);
        if(flags&(1U<<key))report("explicit render diagnostic: %s",dkrr_render_keys[key]);
    }
}
int main() {
    ps5_fp_ieee();
    ps5_klog_capture_stderr("[DKR-R game] ");
    // Pipeline caching and persistence will be measured after the first real
    // game test. Native data is confined to the game's own /data/dkrr subtree.
    setenv("MESA_SHADER_CACHE_DISABLE","true",1);
    // RT64 owns native VideoOut. Select SDL's dummy driver before SDL_Init:
    // creating the PS5 SDL video device would call unavailable keyboard/IME
    // imports even when no keyboard or text entry is being used.
    setenv("SDL_VIDEODRIVER","dummy",1);
    // The public SDL PS5 driver's default map omits Back. DKR-R uses that
    // logical button for its settings overlay, so map the touchpad click to it.
    setenv("SDL_GAMECONTROLLERCONFIG",dkrr_ps5_controller_mapping,1);
    report("build=optional-output-v1; OFL + US single/dual + neutral audio + optional controls");
    configure_render_diagnostics();
    dkrr_ps5_initialize_display();
    const auto output = dkrr_ps5_boot_display_choice();
    report("display requested: %up / %u Hz; SDR; HDMI VRR/ALLM governed by system", output.resolution, output.refresh);
    const char*cache_option=getenv("DKR_PS5_SHADER_CACHE");
    if(cache_option && !strcmp(cache_option,"1")){
        const char*cache="/data/dkrr/renderer/radv-shader-cache";
        if(dkrr_prepare_owned_shader_cache(cache)){
            setenv("MESA_SHADER_CACHE_DIR",cache,1);
            setenv("MESA_DISK_CACHE_DATABASE","true",1);
            setenv("MESA_SHADER_CACHE_MAX_SIZE","64M",1);
            setenv("MESA_SHADER_CACHE_DISABLE","false",1);
            report("explicit shader cache enabled: own exclusive RADV database; 64M target");
        }else report("explicit shader cache refused: own directory unavailable; remains disabled");
    }
    report("function imports unresolved=%u (including dormant optional features)",dkrr_report_missing_imports(missing_import));
    report("process uid=%u gid=%u",unsigned(getuid()),unsigned(getgid()));
    {
        const char *path="/data/dkrr/profile-us-v77/saves/dkr.us.v77.bin.lock";
        struct stat info={};errno=0;int rc=stat(path,&info);
        report("own lock stat rc=%d errno=%d mode=%o nlink=%u dev=%llu ino=%llu",rc,errno,unsigned(info.st_mode),unsigned(info.st_nlink),(unsigned long long)info.st_dev,(unsigned long long)info.st_ino);
        int fd=open(path,O_RDONLY|O_NOFOLLOW);
        if(fd>=0) {rc=fstat(fd,&info);close(fd);report("own lock fstat rc=%d mode=%o nlink=%u",rc,unsigned(info.st_mode),unsigned(info.st_nlink));}
        rc=sceKernelStat(path,&info);
        report("own lock SCE stat rc=0x%08x mode=%o nlink=%u",unsigned(rc),unsigned(info.st_mode),unsigned(info.st_nlink));
    }
    for(const char *path:{"/","/data","/data/dkrr","/app0"}) {
        struct stat info={};errno=0;
        int rc=lstat(path,&info);
        report("lstat path=%s rc=%d errno=%d mode=%o owner=%u",path,rc,errno,unsigned(info.st_mode),unsigned(info.st_uid));
    }
    const char *v77="/data/dkrr/roms/dkr.us.v77.z64";
    const char *v80="/data/dkrr/roms/dkr.us.v80.z64";
#ifndef DKR_PAYLOAD_V77_ENABLED
#define DKR_PAYLOAD_V77_ENABLED 1
#endif
#ifndef DKR_PAYLOAD_V80_ENABLED
#define DKR_PAYLOAD_V80_ENABLED 1
#endif
    unsigned revision=dkrr_ps5_initial_revision();
    dkrr_ps5_set_active_revision(revision);
    const char *rom=revision==80?v80:v77;
    const char *profile=revision==80?"/data/dkrr/profile-us-v80":"/data/dkrr/profile-us-v77";
    int status=3;
    if((revision==77 && !DKR_PAYLOAD_V77_ENABLED)||(revision==80 && !DKR_PAYLOAD_V80_ENABLED)) {
        report("requested revision %u is not included in this own-ROM build",revision);
    } else if(!ordinary_rom(rom)) {
        report("missing own ROM: %s (required canonical 12 MiB; no ROM included)",rom);
    } else {
        FILE *check=std::fopen(rom,"rb");
        report("own ROM fopen=%s errno=%d",check?"ok":"failed",errno);
        if(check)std::fclose(check);
        errno=0;int fd=open(rom,O_RDONLY);
        report("own ROM POSIX open=%d errno=%d",fd,errno);
        if(fd>=0)close(fd);
        errno=0;fd=sceKernelOpen(rom,O_RDONLY,0);
        report("own ROM SCE open=0x%08x errno=%d",unsigned(fd),errno);
        if(fd>=0)sceKernelClose(fd);
        // The game performs its own full revision/content validation before boot.
        char name[]="/app0/eboot.bin",rom_flag[]="--rom",config_flag[]="--config";
        char *args[]={name,rom_flag,const_cast<char *>(rom),config_flag,const_cast<char *>(profile),nullptr};
        try { status=dkrr_game_main(5,args); }
        catch(const std::exception &e) { report("runtime exception: %s",e.what());status=2; }
        if(status==0 && runtime_error_reported.load(std::memory_order_relaxed))status=5;
    }
    struct ps5_heap_stats stats={};ps5_heap_stats(&stats);
    report("completed status=%d revision=%u heap-peak=%zu heap-libc-fallbacks=%llu",status,revision,stats.peak_bytes,stats.libc_fallbacks);
    usleep(200000);
    report("self-close returned rc=0x%08x",unsigned(dkrr_close_this_title()));
    for(;;)usleep(100000);
}
