/* SPDX-License-Identifier: GPL-3.0-or-later
 * Small own-session journal and bounded present-return measurement.
 * Signal handler: fixed local formatting and one write to an already open FD.
 * No stack walk, allocation, lock, printf, fsync or recovery attempt in handler.
 */
#define _GNU_SOURCE
#include "ps5_diagnostics.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <ucontext.h>
#if defined(__PROSPERO__)
#include <ps5platform/context.h>
extern int sigaction(int,const struct sigaction *,struct sigaction *) __attribute__((weak));
extern int sigemptyset(sigset_t *) __attribute__((weak));
#endif
#ifndef DKR_DIAG_DATA_ROOT
#define DKR_DIAG_DATA_ROOT "/data/dkrr"
#endif
_Static_assert(ATOMIC_LLONG_LOCK_FREE == 2, "Diagnostic counters must be lock-free");
_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Signal-stage access must be lock-free");
static int journal=-1;
static char build_id[65]="unversioned";
static uintptr_t entry_address;
static _Atomic unsigned current_stage=DKR_DIAG_ENV;
static _Atomic unsigned journal_bytes;
static _Atomic unsigned journal_error;
static _Atomic unsigned capture_enabled, capture_complete, capture_reported;
static atomic_flag writer=ATOMIC_FLAG_INIT;
static _Atomic uint64_t started, first_present, previous, last, samples, maximum, over33, over50, over100, dropped;
enum { CAPTURE_GAP_LIMIT=64 };
static struct {uint64_t at_ns,gap_ns;} capture_gaps[CAPTURE_GAP_LIMIT];
static unsigned capture_gap_count;
static pthread_once_t ui_once=PTHREAD_ONCE_INIT;
static pthread_t ui_thread;
static _Atomic unsigned ui_observer_started,ui_observer_stop;
static _Atomic uint64_t ui_checkpoint;
static _Atomic uint64_t render_checkpoint;
static _Atomic uint64_t ui_completed,present_completed;
enum { UI_TIMING_PHASES=14 };
static _Atomic uint64_t ui_slow_count[UI_TIMING_PHASES],ui_slow_max[UI_TIMING_PHASES];
static _Atomic uint64_t short_present_previous,short_present_count,short_present_max;
static void update_max(_Atomic uint64_t *value,uint64_t next){
    uint64_t old=atomic_load_explicit(value,memory_order_relaxed);
    while(old<next&&!atomic_compare_exchange_weak_explicit(value,&old,next,memory_order_relaxed,memory_order_relaxed)){}
}
static uint64_t now_ns(void) {
    struct timespec t;
    return clock_gettime(CLOCK_MONOTONIC,&t)==0 ? (uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec : 0;
}
void dkrr_diag_render_wait(unsigned phase){
    const uint64_t now=phase?now_ns()/1000000ULL:0;
    atomic_store_explicit(&render_checkpoint,now?(now<<8)|(phase&255U):0,memory_order_relaxed);
}
extern int dkrr_net_diagnostic(char*,size_t) __attribute__((weak));
extern int dkrr_platform_diagnostic(char*,size_t) __attribute__((weak));
extern int dkrr_async_log_diagnostic(char*,unsigned) __attribute__((weak));
static void *ui_observer(void *unused){
    (void)unused;uint64_t last_report=0,blocked_checkpoint=0,seen_present=0,present_changed=0,timing_report=0,resource_report=0;unsigned reports=0,timing_lines=0,resource_lines=0;
    while(!atomic_load_explicit(&ui_observer_stop,memory_order_acquire)){
        const struct timespec delay={0,250000000};nanosleep(&delay,NULL);
        const uint64_t point=atomic_load_explicit(&ui_checkpoint,memory_order_acquire);
        const uint64_t now=now_ns()/1000000ULL,at=point>>8;
        if((!timing_report||now-timing_report>=10000)&&timing_lines<64){
            timing_report=now;
            for(unsigned phase=1;phase<UI_TIMING_PHASES&&timing_lines<64;++phase){
                const uint64_t count=atomic_exchange(&ui_slow_count[phase],0);
                const uint64_t maximum=atomic_exchange(&ui_slow_max[phase],0);
                if(count){char line[160];snprintf(line,sizeof(line),"DKRR_UI_SLOW v=1 phase=%u samples=%llu max-ms=%llu\n",phase,(unsigned long long)count,(unsigned long long)maximum);dkrr_diag_note(line);++timing_lines;}
            }
            const uint64_t count=atomic_exchange(&short_present_count,0);
            const uint64_t maximum=atomic_exchange(&short_present_max,0);
            if(count&&timing_lines<64){char line[160];snprintf(line,sizeof(line),"DKRR_PRESENT_SLOW v=1 samples=%llu max-us=%llu\n",(unsigned long long)count,(unsigned long long)(maximum/1000));dkrr_diag_note(line);++timing_lines;}
        }
        if((!resource_report||now-resource_report>=30000)&&resource_lines<160){
            resource_report=now;
            if(dkrr_net_diagnostic){char line[224];int bytes=dkrr_net_diagnostic(line,sizeof(line));
                if(bytes>0&&(size_t)bytes<sizeof(line)){dkrr_diag_note(line);++resource_lines;}}
            if(dkrr_platform_diagnostic){char line[224];int bytes=dkrr_platform_diagnostic(line,sizeof(line));
                if(bytes>0&&(size_t)bytes<sizeof(line)){dkrr_diag_note(line);++resource_lines;}}
            if(dkrr_async_log_diagnostic){char line[224];int bytes=dkrr_async_log_diagnostic(line,sizeof(line));
                if(bytes>0&&(size_t)bytes<sizeof(line)){dkrr_diag_note(line);++resource_lines;}}
        }
        if(point&&now>=at&&now-at>=1500){
            if(reports<32&&(!last_report||now-last_report>=10000)){
                char line[192];snprintf(line,sizeof(line),"DKRR_UI_STALL v=1 phase=%u blocked-ms=%llu\n",(unsigned)(point&255),(unsigned long long)(now-at));
                dkrr_diag_note(line);last_report=now;blocked_checkpoint=point;++reports;
            }
        }else if(blocked_checkpoint&&point!=blocked_checkpoint){
            dkrr_diag_note("DKRR_UI_RECOVERED v=1\n");blocked_checkpoint=0;
        }
        const uint64_t presents=atomic_load_explicit(&present_completed,memory_order_relaxed);
        if(presents!=seen_present){seen_present=presents;present_changed=now;}
        else if(presents&&now>=present_changed&&now-present_changed>=2000&&reports<32&&
                (!last_report||now-last_report>=10000)){
            const uint64_t render=atomic_load_explicit(&render_checkpoint,memory_order_relaxed);
            const uint64_t wait_ms=render&&now>=(render>>8)?now-(render>>8):0;
            char line[224];snprintf(line,sizeof(line),"DKRR_PRESENT_STALL v=1 gap-ms=%llu ui-phase=%u ui-completed=%llu presents=%llu render-wait=%u wait-ms=%llu\n",(unsigned long long)(now-present_changed),(unsigned)(point&255),(unsigned long long)atomic_load(&ui_completed),(unsigned long long)presents,(unsigned)(render&255),(unsigned long long)wait_ms);
            dkrr_diag_note(line);last_report=now;++reports;
        }
    }
    return NULL;
}
static void ui_observer_start(void){
    if(pthread_create(&ui_thread,NULL,ui_observer,NULL)==0)atomic_store(&ui_observer_started,1);
    else dkrr_diag_note("DKRR_UI_OBSERVER unavailable\n");
}
void dkrr_diag_ui_phase(unsigned phase){
    const uint64_t now=now_ns()/1000000ULL;
    const uint64_t before=atomic_exchange_explicit(&ui_checkpoint,phase&&now?(now<<8)|(phase&255U):0,memory_order_acq_rel);
    const unsigned previous_phase=(unsigned)(before&255U);
    if(before&&previous_phase<UI_TIMING_PHASES&&now>=(before>>8)&&now-(before>>8)>=8){
        atomic_fetch_add_explicit(&ui_slow_count[previous_phase],1,memory_order_relaxed);
        update_max(&ui_slow_max[previous_phase],now-(before>>8));
    }
    if(!phase)atomic_fetch_add_explicit(&ui_completed,1,memory_order_relaxed);
    if(phase&&!atomic_load(&ui_observer_stop))(void)pthread_once(&ui_once,ui_observer_start);
}
static int directory(const char *path) { struct stat st; return lstat(path,&st)==0 && S_ISDIR(st.st_mode); }
static size_t literal(char *out,size_t at,size_t capacity,const char *s) {
    for(size_t i=0;s[i];++i){if(at==capacity)return capacity;out[at++]=s[i];}return at;
}
static size_t hex(char *out,size_t at,size_t capacity,uint64_t value) {
    const char digits[]="0123456789abcdef";
    for(unsigned i=0;i<16;++i){if(at==capacity)return capacity;out[at++]=digits[(value>>((15-i)*4))&15];}return at;
}
size_t dkrr_diag_format_crash(char *out,size_t capacity,const char *build,const struct dkrr_diag_crash *c) {
    if(!out||!c||!build||capacity<512)return 0;
    size_t at=literal(out,0,capacity,"DKRR_CRASH v=1 build=");
    for(unsigned i=0;i<64 && build[i];++i){const char ch=build[i];if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='-'))return 0;out[at++]=ch;}
    const char *keys[]={" signal="," code="," address="," rip="," rsp="," trap="," error="," entry="," stage="};
    const uint64_t values[]={c->signal,c->code,c->address,c->rip,c->rsp,c->trap,c->error,c->entry,c->stage};
    for(unsigned i=0;i<9;++i){at=literal(out,at,capacity,keys[i]);at=literal(out,at,capacity,"0x");at=hex(out,at,capacity,values[i]);}
    out[at++]='\n'; return at;
}
static void fatal_signal(int number,siginfo_t *info,void *opaque) {
    struct dkrr_diag_crash c={0};
    c.signal=(unsigned)number;c.code=info?(uint64_t)(int64_t)info->si_code:0;c.address=info?(uintptr_t)info->si_addr:0;
    c.entry=entry_address;c.stage=atomic_load_explicit(&current_stage,memory_order_relaxed);
    if(opaque){ucontext_t *context=opaque;
#if defined(__PROSPERO__)
        c.rip=context->uc_mcontext.mc_rip;c.rsp=context->uc_mcontext.mc_rsp;
        c.trap=context->uc_mcontext.mc_trapno;c.error=context->uc_mcontext.mc_err;
#elif defined(__linux__) && defined(__x86_64__)
        c.rip=(uint64_t)context->uc_mcontext.gregs[REG_RIP];c.rsp=(uint64_t)context->uc_mcontext.gregs[REG_RSP];
        c.trap=(uint64_t)context->uc_mcontext.gregs[REG_TRAPNO];c.error=(uint64_t)context->uc_mcontext.gregs[REG_ERR];
#endif
    }
    char line[512];const size_t bytes=dkrr_diag_format_crash(line,sizeof(line),build_id,&c);
    if(journal>=0 && bytes)(void)write(journal,line,bytes);
    /* SA_RESETHAND restored the default disposition on entry. Queue the same
     * signal while it is blocked, so deliberate raise(SIGABRT) also terminates
     * normally after return. kill/getpid/write are async-signal-safe. */
    (void)kill(getpid(),number);
}
const char *dkrr_diag_build(void){return build_id;}
int dkrr_diag_journal_ready(void){return journal>=0&&!atomic_load(&journal_error);}
void dkrr_diag_stage(unsigned stage){atomic_store_explicit(&current_stage,stage,memory_order_relaxed);}
void dkrr_diag_note(const char *line){
    if(journal<0||!line)return;
    const size_t length=strnlen(line,768);
    const unsigned old=atomic_fetch_add_explicit(&journal_bytes,(unsigned)length,memory_order_relaxed);
    if(old<=65536 && length<=65536-old){
        size_t at=0;
        while(at<length){const ssize_t n=write(journal,line+at,length-at);
            if(n<0&&errno==EINTR)continue;
            if(n<=0){atomic_store(&journal_error,1);break;}at+=(size_t)n;
        }
    }else atomic_store(&journal_error,1);
}
int dkrr_diag_initialize(const char *build,uintptr_t entry){
    if(journal>=0)return 0;
    if(!build||!build[0]||strnlen(build,65)>64){errno=EINVAL;return -1;}
    for(const char *p=build;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='-')){errno=EINVAL;return -1;}
    memcpy(build_id,build,strlen(build)+1);entry_address=entry;
    atomic_store(&journal_bytes,0);atomic_store(&journal_error,0);
    if(!directory(DKR_DIAG_DATA_ROOT)){errno=ENOTDIR;return -1;}
    const char *dir=DKR_DIAG_DATA_ROOT "/diagnostics";
    if(mkdir(dir,0700)!=0&&errno!=EEXIST)return -1;
    if(!directory(dir)){errno=ENOTDIR;return -1;}
    struct timeval tv;if(gettimeofday(&tv,NULL)!=0)return -1;
    char path[256];const int n=snprintf(path,sizeof(path),"%s/run-%llu-%06u-%u.log",dir,(unsigned long long)tv.tv_sec,(unsigned)tv.tv_usec,(unsigned)getpid());
    if(n<0||(size_t)n>=sizeof(path)){errno=ENAMETOOLONG;return -1;}
    journal=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_APPEND,0600);
    if(journal<0)return -1;
    char line[240];snprintf(line,sizeof(line),"DKRR_SESSION v=1 build=%s pid=%u entry=0x%016llx\n",build_id,(unsigned)getpid(),(unsigned long long)entry);
    dkrr_diag_note(line);
#if defined(__PROSPERO__)
    if(!sigaction||!sigemptyset){dkrr_diag_note("DKRR_SIGNAL_CAPTURE unavailable imports; OS logging retained\n");return 0;}
#endif
    struct sigaction action={0},old;
    action.sa_sigaction=fatal_signal;action.sa_flags=SA_SIGINFO|SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    const int signals[]={SIGSEGV,SIGBUS,SIGILL,SIGFPE,SIGABRT};
    for(unsigned i=0;i<sizeof(signals)/sizeof(signals[0]);++i){
        /* Preserve an existing framework/loader handler instead of replacing it. */
        if(sigaction(signals[i],NULL,&old)==0 && old.sa_handler==SIG_DFL)(void)sigaction(signals[i],&action,NULL);
    }
    return 0;
}
void dkrr_diag_shutdown(void){
    atomic_store_explicit(&ui_observer_stop,1,memory_order_release);
    if(atomic_exchange(&ui_observer_started,0))pthread_join(ui_thread,NULL);
    dkrr_diag_stage(DKR_DIAG_EXIT);dkrr_diag_capture_finish();dkrr_diag_note("DKRR_SESSION_END\n");if(journal>=0){close(journal);journal=-1;}
}
int dkrr_diag_capture_start_at(uint64_t now){
    if(!now)return -1;
    atomic_store_explicit(&capture_enabled,0,memory_order_release);
    if(atomic_flag_test_and_set_explicit(&writer,memory_order_acquire))return -1;
    capture_gap_count=0;
    atomic_store(&started,now);atomic_store(&first_present,0);atomic_store(&previous,0);atomic_store(&last,0);atomic_store(&samples,0);atomic_store(&maximum,0);
    atomic_store(&over33,0);atomic_store(&over50,0);atomic_store(&over100,0);atomic_store(&dropped,0);atomic_store(&capture_complete,0);atomic_store(&capture_reported,0);
    atomic_store_explicit(&capture_enabled,1,memory_order_release);atomic_flag_clear_explicit(&writer,memory_order_release);return 0;
}
int dkrr_diag_capture_start(void){return dkrr_diag_capture_start_at(now_ns());}
void dkrr_diag_present_at(uint64_t now){
    dkrr_diag_stage(DKR_DIAG_PRESENT);
    if(!atomic_load_explicit(&capture_enabled,memory_order_acquire))return;
    if(atomic_flag_test_and_set_explicit(&writer,memory_order_acquire)){atomic_fetch_add(&dropped,1);return;}
    if(!atomic_load_explicit(&capture_enabled,memory_order_acquire)){atomic_flag_clear_explicit(&writer,memory_order_release);return;}
    const uint64_t start=atomic_load(&started);
    if(now<start||now-start>=60000000000ULL){atomic_store(&capture_enabled,0);atomic_store(&capture_complete,1);}
    else {
        const uint64_t prev=atomic_load(&previous);
        if(prev&&now>prev){const uint64_t gap=now-prev;atomic_fetch_add(&samples,1);
            if(gap>atomic_load(&maximum))atomic_store(&maximum,gap);
            if(gap>33333333){
                atomic_fetch_add(&over33,1);
                if(capture_gap_count<CAPTURE_GAP_LIMIT){capture_gaps[capture_gap_count].at_ns=now-start;capture_gaps[capture_gap_count++].gap_ns=gap;}
            }
            if(gap>50000000)atomic_fetch_add(&over50,1);
            if(gap>100000000)atomic_fetch_add(&over100,1);
        }else if(prev)atomic_fetch_add(&dropped,1);
        if(!prev)atomic_store(&first_present,now);
        if(!prev||now>prev){atomic_store(&previous,now);atomic_store(&last,now);}
    }
    atomic_flag_clear_explicit(&writer,memory_order_release);
}
void dkrr_diag_present(void){
    atomic_fetch_add_explicit(&present_completed,1,memory_order_relaxed);
    dkrr_diag_stage(DKR_DIAG_PRESENT);
    const uint64_t now=now_ns(),before=atomic_exchange_explicit(&short_present_previous,now,memory_order_relaxed);
    if(before&&now>before&&now-before>33333333){
        atomic_fetch_add_explicit(&short_present_count,1,memory_order_relaxed);
        update_max(&short_present_max,now-before);
    }
    if(atomic_load_explicit(&capture_enabled,memory_order_relaxed))dkrr_diag_present_at(now);
}
struct dkrr_diag_capture dkrr_diag_capture_state(void){
    const uint64_t end=atomic_load(&last),start=atomic_load(&first_present);
    /* Interval elapsed starts at the first present, not at the UI button. */
    const uint64_t count=atomic_load(&samples);
    struct dkrr_diag_capture c={count,end>start?end-start:0,atomic_load(&maximum),atomic_load(&over33),atomic_load(&over50),atomic_load(&over100),atomic_load(&dropped),atomic_load(&capture_enabled),atomic_load(&capture_complete)};
    return c;
}
int dkrr_diag_capture_finish(void){
    atomic_store_explicit(&capture_enabled,0,memory_order_release);
    if(atomic_flag_test_and_set_explicit(&writer,memory_order_acquire))return -1;
    const struct dkrr_diag_capture c=dkrr_diag_capture_state();
    if(c.intervals&&!atomic_exchange(&capture_reported,1)){char text[360];snprintf(text,sizeof(text),"DKRR_CAPTURE v=1 build=%s intervals=%llu elapsed-ns=%llu max-gap-ns=%llu over33=%llu over50=%llu over100=%llu dropped=%llu\n",build_id,(unsigned long long)c.intervals,(unsigned long long)c.elapsed_ns,(unsigned long long)c.max_gap_ns,(unsigned long long)c.over_33ms,(unsigned long long)c.over_50ms,(unsigned long long)c.over_100ms,(unsigned long long)c.dropped);dkrr_diag_note(text);
        for(unsigned i=0;i<capture_gap_count;++i){snprintf(text,sizeof(text),"DKRR_GAP v=1 index=%u at-ns=%llu gap-ns=%llu\n",i,(unsigned long long)capture_gaps[i].at_ns,(unsigned long long)capture_gaps[i].gap_ns);dkrr_diag_note(text);}
    }
    atomic_store(&capture_complete,1);
    atomic_flag_clear_explicit(&writer,memory_order_release);
    return dkrr_diag_journal_ready()?0:-2;
}
