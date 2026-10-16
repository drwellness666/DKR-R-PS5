/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host adapter for the exact console decoder. PCM travels through a bounded
 * local pipe; Python validates paths and writes the private output files.
 */
#include "../installer/dkrr_install.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int dkrr_mkdir(const char*path,unsigned mode){(void)path;(void)mode;return 0;}
int dkrr_write_new(const char*path,const void*data,size_t size,unsigned mode){
 (void)mode;const char*p=strrchr(path,'/');if(!p||size>384000||!size||(size&1))return -1;
 char*end;unsigned long slot=strtoul(p+1,&end,10);if(slot>=32||strcmp(end,".pcm"))return -1;
 unsigned char h[8];for(unsigned i=0;i<4;++i){h[i]=(unsigned char)(slot>>(8*i));h[4+i]=(unsigned char)(size>>(8*i));}
 return fwrite(h,1,8,stdout)==8&&fwrite(data,1,size,stdout)==size?0:-1;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;unsigned revision=(unsigned)strtoul(argv[2],NULL,10);if(revision!=77&&revision!=80)return 2;
 FILE*f=fopen(argv[1],"rb");if(!f)return 3;unsigned char*rom=malloc(DKRR_ROM_SIZE);if(!rom){fclose(f);return 3;}
 if(fread(rom,1,DKRR_ROM_SIZE,f)!=DKRR_ROM_SIZE||fgetc(f)!=EOF){free(rom);fclose(f);return 3;}fclose(f);
 int rc=dkrr_audio_prepare(rom,DKRR_ROM_SIZE,revision,"owned");free(rom);return rc<0?4:0;
}
