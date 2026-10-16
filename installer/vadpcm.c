/* Copyright 2022 Dietrich Epp.
 * Adaptation for DKR-R PS5, 2026.
 * SPDX-License-Identifier: MPL-2.0
 * Algorithm adapted from depp/vadpcm codec/decode.c; see
 * licenses/VADPCM-MPL-2.0.txt. This file contains no original game samples.
 */
#include <stddef.h>
#include <stdint.h>
int dkrr_vadpcm(const unsigned char *data,size_t size,unsigned order,unsigned predictors,const int16_t *book,int16_t *out){
 if(order!=2||!predictors||predictors>16||size%9)return -1;
 int32_t state[8]={0};size_t at=0;
 for(size_t frame=0;frame<size;frame+=9){unsigned scale=data[frame]>>4,predictor=data[frame]&15;if(predictor>=predictors)return -1;
  const int16_t *code=book+predictor*order*8;
  for(unsigned vector=0;vector<2;++vector){int64_t sums[8];for(unsigned i=0;i<8;++i)sums[i]=(int64_t)state[6]*code[i]+(int64_t)state[7]*code[8+i];
   for(unsigned k=0;k<8;++k){unsigned byte=data[frame+1+vector*4+k/2];int n=(int)(k%2?byte&15:byte>>4);if(n>=8)n-=16;
    int64_t residual=(int64_t)n*(1U<<scale);sums[k]+=residual*2048;
    for(unsigned i=0;i<7-k;++i)sums[k+1+i]+=residual*code[8+i];
   }
   for(unsigned i=0;i<8;++i){int64_t n=sums[i]>=0?sums[i]/2048:-((-sums[i]+2047)/2048);state[i]=(int32_t)(n>32767?32767:n< -32768?-32768:n);out[at++]=(int16_t)state[i];}
  }
 }return 0;
}
