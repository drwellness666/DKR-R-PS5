#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Keep frame metrics; gate render-thread disk output behind explicit profiling."""
def compose(text):
    needle='std::fprintf(stderr,"[graphics][detail-cost]'
    if text.count(needle)!=1:raise ValueError('Unexpected detail metrics logging contract')
    return '#include "ps5_render_diagnostics.h"\n'+text.replace(needle,'if(dkrr_detail_log_enabled()) '+needle)
