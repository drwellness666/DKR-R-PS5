/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compose only the reviewed file-only updater and artwork retirement/restore.
 * Check all their inputs before the first change. No title launch, IPC, app.db,
 * loader, mount, ROM, profile or save operation is introduced.
 */
#define main dkrr_bundle_update_main
#include "update_native_probe.c"
#undef main
#define ordinary_parents dkrr_poster_ordinary_parents
#define main dkrr_poster_update_main
#include "retire_title_poster.c"
#undef main
#undef ordinary_parents
#ifndef DKR_KEEP_POSTER
#define DKR_KEEP_POSTER 0
#endif
static int presentation_preflight(void){
 if(!DKR_KEEP_POSTER)return poster_retirement_preflight();
 for(size_t i=0;i<sizeof(poster_guards)/sizeof(poster_guards[0]);++i)
  if(matches(&poster_guards[i],poster_guards[i].path)!=1)return 1;
 for(size_t i=0;i<sizeof(poster_files)/sizeof(poster_files[0]);++i)
  if(matches(&poster_files[i],poster_files[i].path)!=1||matches(&poster_files[i],poster_files[i].backup)!=0)return 1;
 return 0;
}
int main(void){
 setvbuf(stdout,NULL,_IOLBF,0);
 puts(DKR_RESTORE_POSTER?"DKRR PREVIEW ROLLBACK: known files only":"DKRR PREVIEW UPDATE: known files only");
 if(native_update_preflight()!=0||presentation_preflight()!=0){
  puts("DKRR PREVIEW STOP: preflight refused; no files changed");return 1;
 }
 int result=dkrr_bundle_update_main();
 if(result!=0){puts("DKRR PREVIEW STOP: app update incomplete; do not launch");return 2;}
 result=DKR_KEEP_POSTER?0:dkrr_poster_update_main();
 if(result!=0){puts("DKRR PREVIEW STOP: artwork update incomplete; do not launch");return 3;}
 puts(DKR_RESTORE_POSTER?"DKRR PREVIEW ROLLBACK PASS: files staged; launch manually":"DKRR PREVIEW UPDATE PASS: files staged; launch manually");
 return 0;
}
