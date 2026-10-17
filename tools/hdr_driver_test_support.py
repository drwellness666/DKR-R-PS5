# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the actual patched VideoOut registration function with host services."""
import os,pathlib,subprocess

def run(source,work):
 name='videoout_register_locked';at=source.index(name+'(');start=source.index('{',at);end=start+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 function=source[at:end]
 code=r'''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int VkResult;
#define VK_SUCCESS 0
#define VK_ERROR_FORMAT_NOT_SUPPORTED -11
#define VK_ERROR_INITIALIZATION_FAILED -3
#define VK_ERROR_OUT_OF_DEVICE_MEMORY -2
#define VIDEOOUT_BUFFERS 5
#define VIDEOOUT_SIZES 3
#define PS5_VIDEO_OUT_PIXEL_FORMAT_B8G8R8A8_SDR UINT64_C(0x8000000000000000)
#define DKRR_VIDEOOUT_HDR_FORMAT UINT64_C(0x8100070422000000)
#define PS5_VIDEO_OUT_TILING_64KB_R_X 0u
#define PS5_VIDEO_OUT_ATTRIBUTE_BYTES 80
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 4
#define MAP_ANONYMOUS 8
#define MAP_FAILED ((void*)-1)
struct wsi_videoout_size {struct {unsigned width,height;} extent;uint64_t buffer_bytes;};
struct wsi_videoout_output {int handle;struct wsi_videoout_set {const struct wsi_videoout_size *size;bool hdr;uint8_t*buffers;}sets[3];unsigned set_count;};
static struct wsi_videoout_output videoout_output={.handle=7};
struct ps5_video_out_buffer {void*data,*metadata,*reserved[2];};
static int videoout_flip_rate=0,register_result,register_calls;static unsigned last_set,last_start;static uint64_t last_format;
static bool videoout_open_locked(void){return true;}
static void*fake_mmap(void*a,size_t bytes,int p,int f,int fd,int off){(void)a;(void)p;(void)f;(void)fd;(void)off;return calloc(1,bytes);}
#define mmap fake_mmap
static int sceVideoOutSetFlipRate(int h,int r){assert(h==7&&r==0);return 0;}
static void sceVideoOutSetBufferAttribute2(void*a,uint64_t fmt,unsigned tile,unsigned w,unsigned h,uint64_t r1,unsigned r2,uint64_t r3){(void)a;assert(tile==0&&w&&h&&!r1&&!r2&&!r3);last_format=fmt;}
static int sceVideoOutRegisterBuffers2(int h,int set,int first,struct ps5_video_out_buffer*b,int count,void*a,int r,void*o){(void)a;assert(h==7&&count==5&&!r&&!o&&b[0].data);last_set=set;last_start=first;++register_calls;return register_result;}
static VkResult '''+function+r'''
int main(void){
 struct wsi_videoout_size size={{1920,1080},512},other={{3840,2160},512};const struct wsi_videoout_set*set=NULL;
 register_result=-1;assert(videoout_register_locked(&size,true,&set)==VK_ERROR_INITIALIZATION_FAILED);
 assert(videoout_output.set_count==1&&videoout_output.sets[0].size==NULL&&last_set==0&&last_format==DKRR_VIDEOOUT_HDR_FORMAT);
 register_result=0;assert(videoout_register_locked(&size,false,&set)==VK_SUCCESS);
 assert(last_set==1&&last_start==5&&last_format==PS5_VIDEO_OUT_PIXEL_FORMAT_B8G8R8A8_SDR&&!set->hdr);
 assert(videoout_register_locked(&size,false,&set)==VK_SUCCESS&&register_calls==2);
 assert(videoout_register_locked(&size,true,&set)==VK_SUCCESS&&set->hdr&&last_set==2&&last_start==10);
 assert(videoout_register_locked(&other,false,&set)==VK_ERROR_FORMAT_NOT_SUPPORTED&&register_calls==3);
 for(unsigned i=0;i<videoout_output.set_count;++i)free(videoout_output.sets[i].buffers);
 puts("PASS actual HDR/SDR VideoOut registration: distinct formats/sets, partial-failure index quarantine, reuse and bounded 15-buffer limit");
}
'''
 p=pathlib.Path(work);(p/'hdr-registration.c').write_text(code);out=p/'hdr-registration'
 subprocess.run(['clang','-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'hdr-registration.c'),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
