/* SPDX-License-Identifier: GPL-3.0-or-later
 * Explicit opt-in to existing upstream diagnostics; no change to saved settings.
 */
#ifndef DKR_PS5_RENDER_DIAGNOSTICS_H
#define DKR_PS5_RENDER_DIAGNOSTICS_H
#include <stddef.h>
#include <string.h>
enum { DKR_RENDER_DIAGNOSTIC_COUNT = 9 };
static const char *const dkrr_render_keys[DKR_RENDER_DIAGNOSTIC_COUNT] = {
 "track_profile", "water_profile", "interpolation_trace",
 "water_uv_baseline", "water_draw_baseline", "water_scroll_baseline",
 "power_profile", "launcher_profile", "shader_cache"
};
static const char *const dkrr_render_environment[DKR_RENDER_DIAGNOSTIC_COUNT] = {
 "DKR_TRACK_PROFILE", "DKR_WATER_PROFILE", "DKR_INTERPOLATION_TRACE",
 "DKR_WATER_UV_BASELINE", "DKR_WATER_DRAW_BASELINE", "DKR_WATER_SCROLL_BASELINE",
 "DKR_POWER_PROFILE", "DKR_LAUNCHER_PROFILE", "DKR_PS5_SHADER_CACHE"
};
/* Parse atomically: malformed/duplicate keys never partially enable a test. */
static inline int dkrr_parse_render_diagnostics(const char *text,size_t bytes,
                                               unsigned *flags) {
 unsigned value=0,seen=0;
 if(!flags || bytes>512 || (!text && bytes) || (bytes && memchr(text,0,bytes)))return 0;
 for(size_t at=0;at<bytes;) {
  size_t start=at;while(at<bytes && text[at]!='\n')++at;
  size_t length=at-start;if(at<bytes)++at;
  if(length && text[start+length-1]=='\r')--length;
  if(!length || text[start]=='#')continue;
  int matched=0;
  for(unsigned key=0;key<DKR_RENDER_DIAGNOSTIC_COUNT;++key) {
   size_t n=strlen(dkrr_render_keys[key]);
   if(length==n+2 && !memcmp(text+start,dkrr_render_keys[key],n) &&
      text[start+n]=='=' && (text[start+n+1]=='0'||text[start+n+1]=='1')) {
    unsigned bit=1U<<key;if(seen&bit)return 0;seen|=bit;
    if(text[start+n+1]=='1')value|=bit;
    matched=1;break;
   }
  }
  if(!matched)return 0;
 }
 *flags=value;return 1;
}
#endif
