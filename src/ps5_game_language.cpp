// SPDX-License-Identifier: GPL-3.0-or-later
#include "recomp.h"
#include "ps5_game_language_cycle.h"
// The reviewed hook retains the real set_language call and its normal EEPROM
// lifecycle. v0 is the unmodified get_language result; a3 is analogueX, which
// that exact getter does not clobber. Change only the setter argument a0.
extern "C" void dkr_european_language_cycle(uint8_t*, recomp_context* context) {
    context->r4 = dkrr_next_game_language(uint32_t(context->r2), int32_t(uint32_t(context->r7)));
}
