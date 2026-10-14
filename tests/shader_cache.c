/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include "ps5_shader_cache.h"
#include <assert.h>
#include <stdlib.h>
int main(void){
 char folder[]="/tmp/dkrr-shader-cache-XXXXXX";assert(mkdtemp(folder));
 char leaf[256],other[256];snprintf(leaf,sizeof(leaf),"%s/own-cache",folder);
 assert(dkrr_prepare_owned_shader_cache(leaf));assert(dkrr_prepare_owned_shader_cache(leaf));
 struct stat st;assert(lstat(leaf,&st)==0&&S_ISDIR(st.st_mode));
 snprintf(other,sizeof(other),"%s/link-cache",folder);assert(symlink(leaf,other)==0);assert(!dkrr_prepare_owned_shader_cache(other));assert(unlink(other)==0);
 snprintf(other,sizeof(other),"%s/link-parent",folder);assert(symlink(leaf,other)==0);
 char nested[256];assert(strlen(other)+5<sizeof(nested));strcpy(nested,other);strcat(nested,"/new");assert(!dkrr_prepare_owned_shader_cache(nested));assert(unlink(other)==0);
 assert(!dkrr_prepare_owned_shader_cache("relative"));assert(!dkrr_prepare_owned_shader_cache("/tmp/../bad"));assert(!dkrr_prepare_owned_shader_cache("/tmp/./bad"));
 assert(rmdir(leaf)==0);FILE*f=fopen(leaf,"w");assert(f);fputs("unknown",f);fclose(f);assert(!dkrr_prepare_owned_shader_cache(leaf));assert(unlink(leaf)==0);assert(rmdir(folder)==0);
 puts("PASS actual owned-cache directory preparation: repeat, leaf/parent symlink, unknown file and traversal refusal; no driver/console claim");
}
