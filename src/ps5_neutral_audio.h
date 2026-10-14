// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
// The neutral mixer returns original int16 samples exactly. No equalizer state
// advances in that path. Keep the original mixer for any gain, music or tone.
inline bool dkrr_ps5_neutral_audio(std::int16_t* output,const std::int16_t* input,
 std::size_t count,bool equalizer_neutral,float volume,bool custom_music,bool tone_active){
 if(!equalizer_neutral||volume!=1.0F||custom_music||tone_active||(count&1U))return false;
 for(std::size_t i=0;i<count;i+=2){output[i]=input[i+1];output[i+1]=input[i];}
 return true;
}
