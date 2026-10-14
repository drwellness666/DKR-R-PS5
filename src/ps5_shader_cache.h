/* SPDX-License-Identifier: GPL-3.0-or-later
 * Prepare one own cache leaf only; unknown/symlink parents are refused.
 */
#ifndef DKRR_PS5_SHADER_CACHE_H
#define DKRR_PS5_SHADER_CACHE_H
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static inline int dkrr_prepare_owned_shader_cache(const char *path){
 char prefix[256];size_t length=strlen(path);
 if(path[0]!='/'||length>=sizeof(prefix)||strstr(path,"/../")||strstr(path,"/./")||path[length-1]=='/'||(length>=3&&!strcmp(path+length-3,"/.."))||(length>=2&&!strcmp(path+length-2,"/."))){errno=EINVAL;return 0;}
 memcpy(prefix,path,length+1);
 for(size_t i=1;i<length;++i)if(prefix[i]=='/'){
  struct stat st;prefix[i]=0;int good=lstat(prefix,&st)==0&&S_ISDIR(st.st_mode);prefix[i]='/';
  if(!good){errno=ENOTDIR;return 0;}
 }
 struct stat st;
 if(lstat(path,&st)!=0){if(errno!=ENOENT||mkdir(path,0777)!=0)return 0;}
 else if(!S_ISDIR(st.st_mode)){errno=ENOTDIR;return 0;}
 int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return 0;
 int good=fstat(fd,&st)==0&&S_ISDIR(st.st_mode);close(fd);return good;
}
#endif
