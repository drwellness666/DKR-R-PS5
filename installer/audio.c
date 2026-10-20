/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original N64 bank parsing, envelope/resampling and bounded local cue mixing.
 * Contains no original game sample data. ADPCM decoder is a separate MPL file.
 */
#include "dkrr_install.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct reader { const unsigned char *p;size_t size;int ok; };
#define DKRR_DECLARE_CONTROLLER_IDS
#include "../src/ps5_controller_audio_bank.h"
extern int dkrr_vadpcm(const unsigned char*,size_t,unsigned,unsigned,const int16_t*,int16_t*);
static const unsigned char *read_at(struct reader *r,uint64_t p,size_t n){if(p>r->size||n>r->size-p){r->ok=0;return NULL;}return r->p+(size_t)p;}
static unsigned byte(struct reader *r,uint64_t p){const unsigned char *b=read_at(r,p,1);return b?b[0]:0;}
static uint32_t u32(struct reader *r,uint64_t p){const unsigned char *b=read_at(r,p,4);return b?((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3]:0;}
static unsigned u16(struct reader *r,uint64_t p){const unsigned char *b=read_at(r,p,2);return b?((unsigned)b[0]<<8)|b[1]:0;}
static double min(double a,double b){return a<b?a:b;}
static double max(double a,double b){return a>b?a:b;}
static int cue(struct reader *r,uint64_t bank,uint64_t samples,uint64_t instrument,uint64_t sfx,uint64_t end,unsigned rate,unsigned id,const char *dest,unsigned slot){
 if(sfx+10U*(id+1)>end)return -1;unsigned index=u16(r,sfx+10U*id),seen[16],count=0;
 double gain=byte(r,sfx+10U*id+2)/128.0,pitch=byte(r,sfx+10U*id+4)/100.0;
 int64_t *mix=calloc(192000,sizeof(*mix));if(!mix)return -1;size_t mixed=0;int result=-1;
 unsigned sound_count=u16(r,instrument+14);
 while(index&&r->ok){if(index>sound_count||count==16)goto done;for(unsigned i=0;i<count;++i)if(seen[i]==index)goto done;seen[count++]=index;
  uint64_t sound=bank+u32(r,instrument+16+4U*(index-1));
  uint64_t env=bank+u32(r,sound),key=bank+u32(r,sound+4),wave=bank+u32(r,sound+8);
  uint64_t base=samples+u32(r,wave);unsigned length=u32(r,wave+4),kind=byte(r,wave+8);
  if(!r->ok||!length||length>2*1024*1024)goto done;
  size_t frames=kind==0?(length/9)*16:length/2;if(!frames)goto done;int16_t *pcm=malloc(frames*sizeof(*pcm));if(!pcm)goto done;
  if(kind==0){uint64_t book=bank+u32(r,wave+16);unsigned order=u32(r,book),predictors=u32(r,book+4);int16_t coefficients[256];
   if(order!=2||predictors<1||predictors>16||length%9>1){free(pcm);goto done;}
   for(unsigned i=0;i<order*predictors*8;++i)coefficients[i]=(int16_t)u16(r,book+8+2U*i);
   const unsigned char *data=read_at(r,base,length-length%9);
   if(!r->ok||!data||dkrr_vadpcm(data,length-length%9,order,predictors,coefficients,pcm)<0){free(pcm);goto done;}
  }else if(kind==1){if(length%2){free(pcm);goto done;}for(size_t i=0;i<frames;++i)pcm[i]=(int16_t)u16(r,base+2*i);}
  else {free(pcm);goto done;}
  const unsigned char *kd=read_at(r,key,6);if(!r->ok||!kd){free(pcm);goto done;}
  double speed=pitch*pow(2.0,((int)kd[4]*100-6000+(int8_t)kd[5])/1200.0);
  if(!__builtin_isfinite(speed)||speed<0.1||speed>10||rate<8000||rate>96000){free(pcm);goto done;}
  int32_t attack=(int32_t)u32(r,env),decay=(int32_t)u32(r,env+4);
  unsigned av=byte(r,env+12),dv=byte(r,env+13);
  double volume=byte(r,sound+13)/127.0*gain;
  size_t total=(size_t)min(144000.0,(double)frames*48000/(rate*speed));
  size_t delay=(size_t)min(48000.0,(double)kd[1]*33333*48000/1000000);
  if(!r->ok){free(pcm);goto done;}if(mixed<delay+total)mixed=delay+total;
  for(size_t i=0;i<total;++i){double pos=(double)i*rate*speed/48000;size_t j=(size_t)pos;if(j>=frames){free(pcm);goto done;}
   double f=pos-j,sample=pcm[j]*(1-f)+pcm[j+1<frames?j+1:j]*f;
   double us=(double)i*1000000/48000,envelope;
   if(attack>0&&us<attack)envelope=av/127.0*us/attack;
   else if(decay>0)envelope=(av+((int)dv-(int)av)*min(1,max(0,us-max(0,attack))/decay))/127;
   else envelope=av/127.0;
   double fade=max(0,min(1,min((double)i/96,(double)(total-1-i)/240)));
   mix[delay+i]+=(int64_t)nearbyint(sample*volume*envelope*fade);
  }free(pcm);index=kd[0]+(kd[2]&0xc0)*4;
 }
 if(!r->ok||!mixed)goto done;
 /* Keep complete one-shot effects. The decoder's three-second source limit
  * already bounds long looping effects; category-specific cuts clip horns,
  * pickups and gold-balloon tails in the middle of the original sound. */
 int64_t peak=0;for(size_t i=0;i<mixed;++i){int64_t n=mix[i]<0?-mix[i]:mix[i];if(n>peak)peak=n;}if(!peak)goto done;
 double scale=min(1,30000.0/(double)peak);int16_t *out=malloc(mixed*2);if(!out)goto done;
 for(size_t i=0;i<mixed;++i)out[i]=(int16_t)nearbyint((double)mix[i]*scale*(slot<9?1:min(1,(double)(mixed-1-i)/240)));
 char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/%u.pcm",dest,slot)>=(int)sizeof(path)){free(out);goto done;}
 result=dkrr_write_new(path,out,mixed*2,0644);free(out);
 done:free(mix);return result;
}
#include "controller_jingle.inl"
int dkrr_audio_prepare(const unsigned char *rom,size_t size,unsigned revision,const char *dest){
 if(size!=DKRR_ROM_SIZE||(revision!=77&&revision!=80))return -1;
#if defined(__x86_64__)
 // ELF receivers may inherit their caller's floating-point mode. Keep cue
 // resampling deterministic, including nearbyint's ties-to-even behavior.
 __builtin_ia32_ldmxcsr(__builtin_ia32_stmxcsr() & ~0xe040U);
#endif
 struct reader r={rom,size,1};uint64_t base=revision==77?969568:970976;
 unsigned count=u32(&r,base);if(count!=50)return -1;uint64_t start=base+((count*4+4+15)/16)*16;
 uint64_t audio=start+u32(&r,base+4*40),table=start+u32(&r,base+4*39);
 uint64_t bank=audio+u32(&r,table+4),samples=audio+u32(&r,table+8),sfx=audio+u32(&r,table+24),end=audio+u32(&r,table+28);
 if(!r.ok||u16(&r,bank)!=0x4231||sfx>=end||end>size)return -1;
 uint64_t b=bank+u32(&r,bank+4);unsigned rate=u32(&r,b+4);uint64_t instrument=bank+u32(&r,b+12);
 if(!r.ok||dkrr_mkdir(dest,0755)<0)return -1;
 for(unsigned i=0;i<DKRR_CONTROLLER_SFX_COUNT;++i)if(cue(&r,bank,samples,instrument,sfx,end,rate,dkrr_controller_sound_ids[i],dest,i)<0){fprintf(stderr,"[controller-audio] effect %u failed\n",i);return -1;}
 for(unsigned i=0;i<8;++i)if(jingle(&r,audio,table,43+i,dest,DKRR_CONTROLLER_SFX_COUNT+i)<0){fprintf(stderr,"[controller-audio] jingle %u failed\n",i);return -1;}
 return 0;
}
