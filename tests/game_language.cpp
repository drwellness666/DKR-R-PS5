// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the upstream EEPROM codec, preserving progress outside its config block.
#include "dkr_save_codec.hpp"
#include "ps5_game_language_cycle.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
int main() {
    namespace codec=dkr::runtime::saves::codec;
    auto image=codec::blank_image();
    image.slots[0].name="DKR";image.slots[0].balloons={12,4,3,2,2,1};
    image.slots[0].cutscenes=0x12345678;image.course_times[0]={3210,"ABC"};
    image.settings.preserved_config_bits=(1ULL<<40)|(1ULL<<24);
    const auto baseline=codec::encode(image);
    // Exercise the adapter against the real codec and the documented retail
    // IDs, including reverse input. Three changes must return to the start.
    for (int direction : {-1, 1}) {
        unsigned language=0;
        const unsigned expected_forward[]={2,1,0};
        const unsigned expected_reverse[]={1,2,0};
        for(unsigned step=0;step<3;++step) {
            language=dkrr_next_game_language(language,direction);
            assert(language==(direction>0?expected_forward[step]:expected_reverse[step]));
            auto draft=image;draft.settings.language=language;
            const auto bytes=codec::encode(draft);
            codec::SaveImage decoded;assert(codec::decode(bytes,decoded));
            assert(codec::validate(bytes) && decoded.settings.language==language);
            assert(std::equal(baseline.begin(),baseline.begin()+0x78,bytes.begin()));
            assert(std::equal(baseline.begin()+0x80,baseline.end(),bytes.begin()+0x80));
            assert(decoded.settings.preserved_config_bits==image.settings.preserved_config_bits);
        }
        assert(language==0);
    }
    for(unsigned language=0;language<3;++language)
        assert(dkrr_next_game_language(language,0)==language);
    assert(dkrr_next_game_language(99,1)==0);
    for(unsigned language=0;language<3;++language) {
        auto edited=image;edited.settings.language=language;
        const auto bytes=codec::encode(edited);
        assert(codec::validate(bytes));
        assert(std::equal(baseline.begin(),baseline.begin()+0x78,bytes.begin()));
        assert(std::equal(baseline.begin()+0x80,baseline.end(),bytes.begin()+0x80));
        codec::SaveImage decoded;assert(codec::decode(bytes,decoded));
        assert(decoded.settings.language==language);
        assert(decoded.settings.preserved_config_bits==image.settings.preserved_config_bits);
        assert(decoded.slots[0].name=="DKR" && decoded.slots[0].cutscenes==0x12345678);
        assert(decoded.slots[0].balloons==image.slots[0].balloons);
        assert(decoded.course_times[0].time==3210 && decoded.course_times[0].initials=="ABC");
        assert((bytes[0x7F]&12)==language*4);
    }
    puts("PASS: forward/reverse three-language cycle; existing EEPROM flags/checksums and untouched progress/records");
}
