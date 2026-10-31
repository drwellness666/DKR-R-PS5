// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_diagnostics.h"
namespace dkrr {
// One present worker per joined renderer session. Observe existing waits;
// never change fence, barrier, queue, pacing or timeout behavior.
template<class Function> decltype(auto) render_wait(unsigned phase,Function&& function) {
    dkrr_diag_render_wait(phase);
    struct Complete {~Complete(){dkrr_diag_render_wait(0);}} complete;
    return function();
}
}
