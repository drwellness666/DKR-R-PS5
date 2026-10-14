/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
/* Retail IDs: English=0, German=1, French=2. The European cycle is
 * English -> French -> German; negative input reverses it. No save is written. */
static inline unsigned dkrr_next_game_language(unsigned current, int direction) {
    if (current > 2) return 0;
    if (direction == 0) return current;
    if (direction > 0) return current == 0 ? 2 : current == 2 ? 1 : 0;
    return current == 0 ? 1 : current == 1 ? 2 : 0;
}
