/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include "ps5_diagnostics.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
static volatile sig_atomic_t handled;
static void preserved(int n){(void)n;handled=1;}
int main(int argc,char **argv){
    if(argc>1){
        struct rlimit limit={0,0};assert(setrlimit(RLIMIT_CORE,&limit)==0);
        if(!strcmp(argv[1],"missing")){assert(dkrr_diag_initialize("integrated-test",(uintptr_t)&main)<0);assert(!dkrr_diag_journal_ready());assert(!strcmp(dkrr_diag_build(),"integrated-test"));return 0;}
        if(!strcmp(argv[1],"preserve")){struct sigaction a={0};a.sa_handler=preserved;sigemptyset(&a.sa_mask);assert(sigaction(SIGABRT,&a,0)==0);}
        assert(dkrr_diag_initialize("integrated-test",(uintptr_t)&main)==0);assert(dkrr_diag_journal_ready());
        dkrr_diag_stage(DKR_DIAG_GAME);
        if(!strcmp(argv[1],"fault")){*(volatile unsigned long *)(uintptr_t)0xd0=42;abort();}
        if(!strcmp(argv[1],"abort")){raise(SIGABRT);return 99;}
        if(!strcmp(argv[1],"preserve")){raise(SIGABRT);assert(handled);dkrr_diag_shutdown();return 0;}
        assert(dkrr_diag_capture_start_at(1)==0);dkrr_diag_present_at(2);dkrr_diag_present_at(100000002);assert(dkrr_diag_capture_finish()==0);
        assert(dkrr_diag_capture_finish()==0);dkrr_diag_shutdown();
        assert(dkrr_diag_initialize("integrated-again",(uintptr_t)&main)==0);dkrr_diag_shutdown();return 0;
    }
    char out[512];struct dkrr_diag_crash c={11,1,0xd0,0x434b53,0x2345,14,4,0x400000,4};
    const size_t n=dkrr_diag_format_crash(out,sizeof(out),"integrated-test",&c);assert(n>0&&n<sizeof(out)&&out[n-1]=='\n');out[n]=0;
    assert(strstr(out,"address=0x00000000000000d0"));assert(strstr(out,"rip=0x0000000000434b53"));
    assert(!dkrr_diag_format_crash(out,511,"test",&c));assert(!dkrr_diag_format_crash(out,sizeof(out),"bad\nname",&c));
    assert(dkrr_diag_capture_start_at(0)<0);
    dkrr_diag_present_at(100);assert(dkrr_diag_capture_state().intervals==0);
    assert(dkrr_diag_capture_start_at(100)==0);dkrr_diag_present_at(100000000);dkrr_diag_present_at(116666667);
    dkrr_diag_present_at(150000001);dkrr_diag_present_at(200000002);dkrr_diag_present_at(300000003);dkrr_diag_present_at(299999999);
    struct dkrr_diag_capture state=dkrr_diag_capture_state();
    assert(state.intervals==4&&state.elapsed_ns==200000003&&state.max_gap_ns==100000001);
    assert(state.over_33ms==3&&state.over_50ms==2&&state.over_100ms==1&&state.dropped==1&&state.active);
    dkrr_diag_present_at(60000000100ULL);state=dkrr_diag_capture_state();assert(!state.active&&state.complete&&state.intervals==4);
    assert(dkrr_diag_capture_finish()==-2);dkrr_diag_present_at(70000000000ULL);assert(dkrr_diag_capture_state().intervals==4);
    assert(dkrr_diag_capture_start_at(500)==0);dkrr_diag_present_at(499);assert(dkrr_diag_capture_state().complete);
    puts("PASS bounded crash formatter and deterministic disabled/60s present recorder");return 0;
}
