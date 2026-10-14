/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ps5_render_diagnostics.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 unsigned value=99;
 const char *good="# optional A/B capture\r\nwater_profile=1\r\ntrack_profile=1\nwater_uv_baseline=0";
 assert(dkrr_parse_render_diagnostics(good,strlen(good),&value)&&value==3);
 const char *bad[]={"water_profile=1\nwater_profile=0", "MESA_SHADER_CACHE_DISABLE=0",
  "water_profile=2", "track_profile=1\nunknown=1", "track_profile=1 ", "\rtrack_profile=1"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i){value=99;assert(!dkrr_parse_render_diagnostics(bad[i],strlen(bad[i]),&value)&&value==99);}
 char embedded[]={'w',0,'1'};assert(!dkrr_parse_render_diagnostics(embedded,sizeof(embedded),&value));
 char huge[513];memset(huge,'#',sizeof(huge));assert(!dkrr_parse_render_diagnostics(huge,sizeof(huge),&value));
 assert(dkrr_parse_render_diagnostics("",0,&value)&&value==0);
 assert(dkrr_parse_render_diagnostics(NULL,0,&value)&&value==0);
 assert(!dkrr_parse_render_diagnostics(NULL,1,&value));
 const char *all="track_profile=1\nwater_profile=1\ninterpolation_trace=1\nwater_uv_baseline=1\nwater_draw_baseline=1\nwater_scroll_baseline=1\npower_profile=1\nlauncher_profile=1\nshader_cache=1\n";
 assert(dkrr_parse_render_diagnostics(all,strlen(all),&value)&&value==511);
 const char *timing="power_profile=1\nlauncher_profile=0";
 assert(dkrr_parse_render_diagnostics(timing,strlen(timing),&value)&&value==64);
 puts("PASS: bounded render diagnostics, atomic rejection, duplicates and exact A/B flags");
}
