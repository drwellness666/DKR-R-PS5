// SPDX-License-Identifier: GPL-3.0-or-later
// Native title entry around the authored DKR-R runtime. ROM data stays external.
#include <ps5platform/fp.h>
#include <ps5platform/klog.h>
#include <ps5platform/heap.h>
#include <ps5platform/exec.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <cerrno>
#include <fcntl.h>
#include <initializer_list>
#include <atomic>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "ps5_net.h"
#include "ps5_controller_mapping.h"
#include "ps5_render_diagnostics.h"
#include "ps5_shader_cache.h"
#include "ps5_revision_selection.h"
#include "ps5_display_options.h"
#include "ps5_hdr_options.h"
#include "ps5_diagnostics.h"
#include "ps5_async_log.h"
#include "ps5_installation_lease.hpp"
#include "ps5_internet_update.hpp"
#include "ps5_native_update.hpp"
#include "ps5_online_platform.h"

int dkrr_game_main(int, char **);
extern "C" void dkrr_ps5_shutdown_dialogs();
extern "C" unsigned dkrr_report_missing_imports(void (*)(const char *));
#include "rtc/rtc.h"
extern "C" int sceKernelSendNotificationRequest(int,void*,size_t,int) __attribute__((weak));
extern "C" int sceKernelOpen(const char *,int,int);
extern "C" int sceKernelClose(int);
extern "C" int sceKernelStat(const char *,struct stat *);
static std::atomic_bool runtime_error_reported=false;
static std::atomic_bool native_network_apis_ready=true;
extern "C" int dkrr_rtc_entropy_available(void);
extern "C" const char* dkrr_rtc_entropy_source(void);
extern "C" bool dkrr_native_online_apis_ready(void) {
    return native_network_apis_ready.load(std::memory_order_relaxed)&&dkrr_rtc_entropy_available();
}
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
    dkrr_diag_note(line);
}
extern "C" int dkrr_platform_diagnostic(char* out,size_t bytes) {
    struct ps5_heap_stats heap{};ps5_heap_stats(&heap);
    uint64_t regions=0,code_bytes=0;ps5_exec_live(&regions,&code_bytes);
    return std::snprintf(out,bytes,"DKRR_MEMORY v=1 mapped=%llu peak=%llu arenas=%u segments=%u libc-fallbacks=%llu code-regions=%llu code-bytes=%llu\n",
        (unsigned long long)heap.mapped_bytes,(unsigned long long)heap.peak_bytes,heap.arenas,heap.segments,
        heap.libc_fallbacks,(unsigned long long)regions,(unsigned long long)code_bytes);
}
static void notify_update(const char* message) {
    if(!sceKernelSendNotificationRequest)return;
    struct {char reserved[45];char text[3075];} notification{};
    std::snprintf(notification.text,sizeof(notification.text),"%s",message);
    sceKernelSendNotificationRequest(0,&notification,sizeof(notification),0);
}
static void report_network_io() {
    if(!native_network_apis_ready.load()){report("native network I/O unavailable: missing SceNet imports");return;}
    const int init=dkrr_net_init_result();
    int fd=dkrr_net_socket(AF_INET,SOCK_DGRAM,0);
    const int socket_error=fd<0?errno:0;
    int nonblock=-1,nonblock_error=0;
    if(fd>=0){
        int enabled=1;errno=0;nonblock=dkrr_net_ioctl(fd,FIONBIO,&enabled);nonblock_error=nonblock<0?errno:0;
        dkrr_net_close(fd);
    }
    int fds[2];const int wake=dkrr_net_wakeup_pair(fds),wake_error=wake<0?errno:0;
    int delivered=0;
    if(!wake){char byte=1;pollfd pfd{fds[0],POLLIN,0};
        delivered=dkrr_net_send(fds[1],&byte,1,0)==1&&dkrr_net_poll(&pfd,1,20)==1&&dkrr_net_recv(fds[0],&byte,1,0)==1;
        dkrr_net_close(fds[0]);dkrr_net_close(fds[1]);}
    report("native SceNet I/O: init=0x%08x socket-error=%d nonblocking=%d/%d UDP-wakeup=%d/%d delivered=%d",unsigned(init),socket_error,
           nonblock,nonblock_error,wake,wake_error,delivered);
}
static void missing_import(const char *name) {
    // Offline play may use none of the SDK's dormant desktop imports. The
    // native lobby must not call missing functions when RTC is first opened.
    static constexpr const char* network[]={"__inet_ntop","__inet_pton","freeifaddrs","getifaddrs","isatty","pthread_getthreadid_np","pthread_rwlock_tryrdlock","pthread_rwlockattr_destroy","pthread_rwlockattr_init","setbuf",
        "sceNetCtlInit","sceNetCtlGetInfo","sceNetInit","sceNetErrnoLoc","sceNetSocket","sceNetSocketClose","sceNetBind","sceNetConnect","sceNetListen","sceNetAccept","sceNetShutdown",
        "sceNetGetsockopt","sceNetSetsockopt","sceNetGetsockname","sceNetGetpeername","sceNetSend","sceNetSendto","sceNetRecv","sceNetRecvfrom","sceNetRecvmsg","sceNetSendmsg",
        "sceNetEpollCreate","sceNetEpollControl","sceNetEpollWait","sceNetEpollDestroy"};
    for(const char* function:network)if(!strcmp(name,function))native_network_apis_ready.store(false,std::memory_order_relaxed);
    report("unresolved function import: %s",name);
}
#define DKR_NATIVE_TITLE_ID "PPSA94678"
#include "ps5_native_close.h"

static bool ordinary_rom(const char *path) {
    struct stat info={};
    return stat(path,&info)==0 && S_ISREG(info.st_mode) && info.st_size==12*1024*1024;
}
static void configure_render_diagnostics() {
    int fd=open("/data/dkrr/render-diagnostics.txt",O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
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
    const int installation_lease=dkrr_runtime_installation_lease();
    if(installation_lease<0) {
        report("installation is busy or the shared application lease is unavailable");
        dkrr_diag_shutdown();dkrr_close_this_title();for(;;)usleep(100000);
    }
    char update_error[192]{};
    const int recovery=dkrr::updates::native::recover(installation_lease,update_error,sizeof(update_error));
    if(recovery!=0) {
        report("native update recovery: %s",update_error);notify_update(update_error);
        close(installation_lease);dkrr_diag_shutdown();dkrr_close_this_title();for(;;)usleep(100000);
    }
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
    report("build=integrated-v1 source=%s; US dual + languages + optional controls/output + diagnostics",dkrr_diag_build());
    configure_render_diagnostics();
    dkrr_ps5_initialize_display();
    dkrr_ps5_initialize_hdr();
    const auto output = dkrr_ps5_boot_display_choice();
    report("display requested: %up / %u Hz; colour=%s; HDMI VRR/ALLM governed by system", output.resolution, output.refresh, dkrr_ps5_hdr_requested()?"HDR10":"SDR");
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
    report("online platform: network-imports=%s secure-rng=%s",native_network_apis_ready.load()?"ready":"missing",dkrr_rtc_entropy_source());
    report_network_io();
    report("process uid=%u gid=%u",unsigned(getuid()),unsigned(getgid()));
    {
        const char *path="/data/dkrr/profile-us-v77/saves/dkr.us.v77.bin.lock";
        struct stat info={};errno=0;int rc=stat(path,&info);
        report("own lock stat rc=%d errno=%d mode=%o nlink=%u dev=%llu ino=%llu",rc,errno,unsigned(info.st_mode),unsigned(info.st_nlink),(unsigned long long)info.st_dev,(unsigned long long)info.st_ino);
        int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
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
        dkrr_diag_stage(DKR_DIAG_GAME);
        rtcInitLogger(RTC_LOG_INFO,nullptr);
        dkrr::updates::initialize();
        report("update startup: %s",dkrr::updates::view().message);
        try { status=dkrr_game_main(5,args); }
        catch(const std::exception &e) { report("runtime exception: %s",e.what());status=2; }
        if(status==0 && runtime_error_reported.load(std::memory_order_relaxed))status=5;
    }
    dkrr_ps5_shutdown_dialogs();
    struct ps5_heap_stats stats={};ps5_heap_stats(&stats);
    dkrr::updates::shutdown();
    report("completed status=%d revision=%u heap-peak=%zu heap-libc-fallbacks=%llu",status,revision,stats.peak_bytes,stats.libc_fallbacks);
    if(dkrr::updates::close_requested()) {
        bool installed=status==0&&dkrr::updates::native::apply(installation_lease,update_error,sizeof(update_error));
        const char* message=installed?"DKR-R updated. Start it again from its PS5 icon.":update_error[0]?update_error:"Update postponed because the game did not close cleanly. Start DKR-R again.";
        report("native update result: %s",message);
        notify_update(message);
    }
    dkrr_async_log_stop();
    dkrr_diag_shutdown();
    close(installation_lease);
    usleep(200000);
    report("self-close returned rc=0x%08x",unsigned(dkrr_close_this_title()));
    for(;;)usleep(100000);
}
