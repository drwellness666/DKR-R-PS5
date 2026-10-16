/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../installer/audio.c"
#include <assert.h>
int dkrr_write_new(const char*a,const void*b,size_t n,unsigned m){(void)a;(void)b;(void)n;(void)m;return 0;}
int dkrr_mkdir(const char*a,unsigned m){(void)a;(void)m;return 0;}
static void put32(unsigned char*p,unsigned v){p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
int main(void){
 unsigned char data[512]={0};struct jevent events[256];unsigned n,division;
 const unsigned char valid[]={0,255,81,7,161,32,0,192,0,0,144,60,100,24,24,255,47};
 put32(data,68);put32(data+64,96);memcpy(data+68,valid,sizeof(valid));
 assert(jparse(data,68+sizeof(valid),events,&n,&division)==0&&n==3&&division==96&&events[2].duration==24&&events[2].a==60);
 for(size_t size=0;size<68+sizeof(valid);++size)assert(jparse(data,size,events,&n,&division)<0);
 for(unsigned div=0;div<2000;++div){put32(data+64,div);int rc=jparse(data,68+sizeof(valid),events,&n,&division);assert((div>0&&div<=960)==(rc==0));}put32(data+64,96);
 // Bad track positions, tempo at a nonzero tick, unsupported loops/system data.
 for(unsigned offset=1;offset<68;++offset){put32(data,offset);assert(jparse(data,85,events,&n,&division)<0);}put32(data,68);
 data[68]=1;assert(jparse(data,85,events,&n,&division)<0);data[68]=0;
 for(unsigned meta=0;meta<255;++meta)if(meta!=81){data[70]=(unsigned char)meta;assert(jparse(data,85,events,&n,&division)<0);}data[70]=81;
 // Invalid back references and overlong VLQ are rejected, never unbounded.
 struct jtrack bad={data,3,0,0,0,1};data[0]=254;data[1]=0;data[2]=1;assert(jb(&bad)==0&&!bad.ok);
 unsigned char long_v[]={128,128,128,128,0};bad=(struct jtrack){long_v,sizeof(long_v),0,0,0,1};jv(&bad);assert(!bad.ok);
 // Reproducible malformed inputs, exercising parser bounds under ASan/UBSan.
 unsigned seed=0x715a;for(unsigned k=0;k<8000;++k){unsigned char fuzz[512];for(unsigned i=0;i<sizeof(fuzz);++i){seed=seed*1664525U+1013904223U;fuzz[i]=(unsigned char)(seed>>24);}put32(fuzz,k%3?68:511);put32(fuzz+64,96);(void)jparse(fuzz,68+k%445,events,&n,&division);}
 puts("PASS bounded CSeq parser: tempo/notes/duration, truncated streams, track/division bounds, unsupported metadata, invalid back references and 8000 malformed-input cases");
}
