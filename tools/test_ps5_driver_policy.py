#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual patched queue-log and bounded vblank code, without GPU."""
import argparse,hashlib,json,os,pathlib,subprocess,tarfile,tempfile,urllib.request
from apply_ps5_driver_policy import apply
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--cache',type=pathlib.Path,action='append',default=[]);a=p.parse_args()
spec=next(x for x in json.loads((root/'VULKAN_SOURCES.json').read_text())['sources'] if x['name']=='PS5_Mesa')
manifest=json.loads((root/'patches/ps5-driver-policy.json').read_text())
def function(text,name):
 at=text.index(name+'(');start=text.index('{',at);depth=1;end=start+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[at:end]
with tempfile.TemporaryDirectory(prefix='dkrr-driver-policy-') as tmp:
 work=pathlib.Path(tmp);archive=next((x/spec['file'] for x in a.cache if (x/spec['file']).is_file()),work/spec['file'])
 if not archive.exists():
  with urllib.request.urlopen(spec['url'],timeout=60) as response:archive.write_bytes(response.read(256*1024*1024+1))
 if hashlib.sha256(archive.read_bytes()).hexdigest()!=spec['sha256']:raise ValueError('Pinned driver archive mismatch')
 mesa=work/'mesa';mesa.mkdir()
 with tarfile.open(archive) as tar:
  for member in tar.getmembers():
   n='/'.join(member.name.split('/')[spec['strip_components']:])
   if member.isfile() and n in manifest['files']:
    target=mesa/n;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(tar.extractfile(member).read())
 apply(mesa);assert apply(mesa)['unchanged']
 cs=(mesa/'src/amd/vulkan/winsys/ps5/radv_ps5_cs.c').read_text();wsi=(mesa/'src/vulkan/wsi/wsi_common_videoout.c').read_text()
 code=r'''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
static bool trace;static uint64_t fake_time;static unsigned clock_calls;
static bool debug_get_bool_option(const char *name,bool fallback){(void)fallback;assert(!strcmp(name,"DKR_POWER_PROFILE"));return trace;}
static uint64_t os_time_get_nano(void){++clock_calls;return fake_time;}
static void radv_ps5_submit_times(uint64_t *times){memset(times,0,3*sizeof(uint64_t));}
static struct {uint64_t window_start,count,words,claim_ns,copy_ns;} radv_ps5_submit_window;
static void '''+function(cs,'radv_ps5_note_submission')+r'''
#define VIDEOOUT_PERIOD_INTERVALS 6
#define VIDEOOUT_HIGH_REFRESH_LIMIT_NS UINT64_C(12500000)
#define PS5_VIDEO_OUT_USER_SYSTEM 0xff
#define PS5_VIDEO_OUT_MODE_HIGH_FRAME_RATE 15u
#define PS5_VIDEO_OUT_MODE_RESTORE 1u
#define VIDEOOUT_ATTRIBUTE3_HIGH_FRAME_RATE 0x40u
static unsigned long attributes;
static unsigned long videoout_title_attribute3(void){return attributes;}
static struct wsi_videoout_output {bool opened, high_frame_rate; int handle;} videoout_output;
static int support_answer,configure_answer,configured_calls,restore_calls;
static int sceVideoOutOpen(int user,int bus,int index,void *p){assert(user==0xff&&bus==0&&index==0&&!p);return 7;}
static int sceVideoOutIsOutputSupported(int h,unsigned mode,void *a,void *b,void *c){assert(h==7&&mode==15&&!a&&!b&&!c);return support_answer;}
static int sceVideoOutConfigureOutput(int h,unsigned mode,void *a,void *b,void *c){assert(h==7&&!a&&!b&&!c);if(mode==1){++restore_calls;return 0;}assert(mode==15);++configured_calls;return configure_answer;}
static unsigned waits;static uint64_t step1,step2;static unsigned fail_at;
static int sceVideoOutWaitVblank(int handle){assert(handle==7);++waits;if(waits==fail_at)return -1;fake_time+=waits<=7?step1:step2;return 0;}
static uint64_t '''+function(wsi,'videoout_vblank_period_ns')+r'''
static uint64_t '''+function(wsi,'videoout_high_refresh_period_ns')+r'''
static bool '''+function(wsi,'videoout_open_locked')+r'''
static void reset(uint64_t a,uint64_t b,unsigned failure){waits=0;fake_time=0;step1=a;step2=b;fail_at=failure;}
static void open_reset(unsigned long attr,int support,int configured,uint64_t step,unsigned failure){
 memset(&videoout_output,0,sizeof(videoout_output));attributes=attr;support_answer=support;configure_answer=configured;configured_calls=restore_calls=0;reset(step,step,failure);
 assert(videoout_open_locked());assert(videoout_open_locked());
}
int main(void){
 trace=false;for(unsigned n=0;n<1000;n++)radv_ps5_note_submission(100,1,1);assert(clock_calls==0&&radv_ps5_submit_window.count==0);
 trace=true;fake_time=1;radv_ps5_note_submission(100,1,1);fake_time=10000000002ULL;radv_ps5_note_submission(100,1,1);assert(clock_calls==2&&radv_ps5_submit_window.count==0);
 reset(16683400,8341600,0);assert(videoout_high_refresh_period_ns(7)==8341600&&waits==14);
 reset(8341600,8341600,0);assert(videoout_high_refresh_period_ns(7)==8341600&&waits==7);
 reset(16683400,16683400,0);assert(videoout_high_refresh_period_ns(7)==16683400&&waits==28);
 reset(16683400,8341600,8);assert(videoout_high_refresh_period_ns(7)==0&&waits==8);
 open_reset(0x40,1,0,16683400,0);assert(!videoout_output.high_frame_rate&&restore_calls==1&&configured_calls==1&&waits==28);
 open_reset(0x40040,1,0,20873000,0);assert(videoout_output.high_frame_rate&&restore_calls==0&&configured_calls==1&&waits==28);
 open_reset(0x40040,1,0,8341600,0);assert(videoout_output.high_frame_rate&&restore_calls==0&&waits==7);
 open_reset(0x40040,1,0,20873000,2);assert(!videoout_output.high_frame_rate&&restore_calls==1&&waits==2);
 open_reset(0x40040,0,0,20873000,0);assert(!videoout_output.high_frame_rate&&configured_calls==0&&waits==0);
 open_reset(0x40040,-7,0,20873000,0);assert(!videoout_output.high_frame_rate&&configured_calls==0&&waits==0);
 open_reset(0x40040,1,-9,20873000,0);assert(!videoout_output.high_frame_rate&&configured_calls==1&&waits==0);
 open_reset(0x40000,1,0,8341600,0);assert(!videoout_output.high_frame_rate&&configured_calls==0&&waits==0);
 puts("PASS actual source: VRR idle cadence retained only after successful HFR negotiation; unsupported/error/fixed-rate fallbacks; quiet default skips queue log/clock bookkeeping; explicit diagnostics retained; transient/steady/error/bounded vblank checks");
}
'''
 (work/'test.c').write_text(code);out=work/'test'
 subprocess.run(['clang','-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(work/'test.c'),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
