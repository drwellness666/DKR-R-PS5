# Copyright 2022 Dietrich Epp.
# Python adaptation for DKR-R PS5, 2026.
# SPDX-License-Identifier: MPL-2.0
# Adapted from https://github.com/depp/vadpcm/blob/main/codec/decode.c
# See licenses/VADPCM-MPL-2.0.txt. This module contains no game data.
def decode(data, order, predictors, book):
    if order != 2 or not 1 <= predictors <= 16 or len(book) != order*predictors*8 or len(data)%9:
        raise ValueError('Unsupported VADPCM codebook/frame layout')
    result=[]; state=[0]*8
    for frame in range(0,len(data),9):
        scale=data[frame]>>4; predictor=data[frame]&15
        if predictor>=predictors:raise ValueError('VADPCM predictor outside codebook')
        code=book[predictor*order*8:(predictor+1)*order*8]
        for vector in range(2):
            sums=[sum(state[8-order+k]*code[k*8+i] for k in range(order)) for i in range(8)]
            for k in range(8):
                byte=data[frame+1+vector*4+k//2]
                nibble=(byte>>4) if k%2==0 else byte&15
                residual=(nibble if nibble<8 else nibble-16)*(1<<scale)
                sums[k]+=residual*2048
                for i in range(7-k):sums[k+1+i]+=residual*code[(order-1)*8+i]
            state=[max(-32768,min(32767,n>>11)) for n in sums]
            result.extend(state)
    return result
