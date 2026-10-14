/* SPDX-License-Identifier: GPL-3.0-or-later
 * Captures one native launch's result and relevant loader text. No console
 * files, settings, process privileges or installed applications are changed.
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/stat.h>

struct launch_ctx { uint32_t size,user,options;uint64_t crash;uint32_t check; };
int sceUserServiceInitialize(void *);
int sceUserServiceGetForegroundUser(uint32_t *);
int sceSystemServiceLaunchApp(const char *,const char **,struct launch_ctx *);
int sceSystemServiceKillApp(int,int,int,int);
static atomic_int finished;
static atomic_int launched_app;
static atomic_int completed;
static void *launch(void *ignored) {
    (void)ignored;
    struct launch_ctx ctx={0};const char *args[]={NULL};
    int init=sceUserServiceInitialize(NULL);
    printf("native-launch: user-service init rc=0x%08x\n",(unsigned)init);
    int rc=sceUserServiceGetForegroundUser(&ctx.user);
    printf("native-launch: foreground-user rc=0x%08x\n",(unsigned)rc);
    if(!rc) {
        rc=sceSystemServiceLaunchApp("PPSA94677",args,&ctx);
        printf("native-launch: sceSystemServiceLaunchApp(PPSA94677) rc=0x%08x\n",(unsigned)rc);
        if(rc>0)atomic_store(&launched_app,rc);
    }
    atomic_store(&finished,1);return NULL;
}
static double now(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9; }
static void line(const char *s) {
    if(strstr(s,"[DKR-R]") && strstr(s,"completed status="))atomic_store(&completed,1);
    if(strstr(s,"rmdir(") || strstr(s,"dirent=") || strstr(s,"remains in FsSandbox"))return;
    if(strncmp(s,"#",1)==0 || strstr(s,"A user thread receives a fatal signal") || strstr(s,"mDBG: Sending signal")) {
        printf("native-klog: %s\n",s);return;
    }
    const char *terms[]={"PPSA94677","DKR-R","plume-probe","[Agc","radv/ps5","sceSystemServiceLaunchApp","[SceLncUtil]","[ld]","[rtld]","[dynlib]","AuthMgr","sys_dynlib","load module","unresolved","elf error","SELF error","Failed to load","failed to load","[Syscore","[sceProcessStarter]","kernel: ","[kstuff]"};
    for(size_t i=0;i<sizeof(terms)/sizeof(terms[0]);++i) {
        if(strstr(s,terms[i])) { printf("native-klog: %s\n",s);return; }
    }
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);
    const char *files[]={"/data/homebrew/PPSA94677/eboot.bin","/data/homebrew/PPSA94677/sce_module/libc.prx"};
    for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) {
        struct stat st;
        if(stat(files[i],&st)==0)printf("native-launch: %s mode=%o bytes=%llu\n",files[i],(unsigned)(st.st_mode&0777),(unsigned long long)st.st_size);
        else printf("native-launch: stat failed: %s\n",strerror(errno));
    }
    int fd=open("/dev/klog",O_RDONLY|O_NONBLOCK);
    if(fd<0) { printf("native-launch: klog unavailable: %s\n",strerror(errno));return 1; }
    char block[4096],current[4096];size_t used=0,total=0;
    // Old messages are discarded, rather than exported as a console inventory.
    while(total<1048576) { ssize_t n=read(fd,block,sizeof(block));if(n<=0)break;total+=(size_t)n;
#if defined(DKR_CAPTURE_ONLY)
        for(ssize_t i=0;i<n;++i) {
            if(block[i]=='\n') { current[used]=0;line(current);used=0; }
            else if(used+1<sizeof(current))current[used++]=block[i];
        }
#endif
    }
#if defined(DKR_CAPTURE_ONLY)
    close(fd);puts("native-klog: existing diagnostic messages captured; no title launched");return 0;
#endif
    puts("native-launch: fresh capture armed; launching only PPSA94677 once");
    pthread_t worker;
    int rc=pthread_create(&worker,NULL,launch,NULL);
    if(rc) { printf("native-launch: thread rc=%d\n",rc);close(fd);return 2; }
    pthread_detach(worker);
    double deadline=now()+40;
    while(now()<deadline) {
        ssize_t n=read(fd,block,sizeof(block));
        if(n<=0) { usleep(20000);continue; }
        for(ssize_t i=0;i<n;++i) {
            if(block[i]=='\n') { current[used]=0;line(current);used=0; }
            else if(used+1<sizeof(current))current[used++]=block[i];
        }
        if(atomic_load(&completed) && atomic_load(&launched_app)>0) {
#if defined(DKR_SELF_CLOSE_TEST)
            // The native title performs its own verified closure in this test.
            // Capture subsequent exit/crash messages, without sending any kill.
            if(deadline>now()+3)deadline=now()+3;
#else
            int app=atomic_load(&launched_app);
            printf("native-launch: closing only own returned app_id=0x%08x through system service\n",(unsigned)app);
            int result=sceSystemServiceKillApp(app,-1,0,0);
            printf("native-launch: system-service close rc=0x%08x\n",(unsigned)result);
            break;
#endif
        }
    }
    close(fd);printf("native-launch: capture ended; launch-call-returned=%d\n",atomic_load(&finished));
    return atomic_load(&finished)?0:3;
}
