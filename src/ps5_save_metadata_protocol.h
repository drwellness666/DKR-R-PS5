/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DKR_SAVE_METADATA_PROTOCOL_H
#define DKR_SAVE_METADATA_PROTOCOL_H
#include <stdint.h>
#include <string.h>
#define DKR_SAVE_METADATA_PORT 9038
#define DKR_SAVE_METADATA_PATH_MAX 512
struct dkrr_metadata_request {uint32_t magic;char path[DKR_SAVE_METADATA_PATH_MAX];};
struct dkrr_metadata_reply {uint32_t magic,status;uint64_t inode,bytes,modified;uint32_t mode,uid,gid,links;};
static inline int dkrr_metadata_allowed(const char *path){
 const char *roots[]={"/data/dkrr/profile-us-v77/","/data/dkrr/profile-us-v80/"};
 const char *relative=0;for(unsigned i=0;i<2;i++){size_t n=strlen(roots[i]);if(!strncmp(path,roots[i],n)){relative=path+n;break;}}
 if(!relative)return 0;
 // Keep the Save Manager's backup-before-edit flow under the same link checks.
 int allowed=!strncmp(relative,"controller-pak-",15) || !strncmp(relative,"pending-development-seed.bin",28);
 const char *folders[]={"saves/","save-backups/","save-history/",
  "save-migration-v1/","save-transactions/","pending-save-transaction/","save-recovery/"};
 for(unsigned i=0;i<sizeof(folders)/sizeof(folders[0]);i++){size_t n=strlen(folders[i]);if(!strncmp(relative,folders[i],n)&&relative[n])allowed=1;}
 if(!allowed)return 0;
 if(strstr(path,"//"))return 0;
 const char *begin=relative;for(const char*p=relative;;p++){unsigned char c=(unsigned char)*p;if(c=='/'||!c){size_t n=(size_t)(p-begin);if(!n||(n==1&&begin[0]=='.')||(n==2&&begin[0]=='.'&&begin[1]=='.'))return 0;begin=p+1;}else if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='_'))return 0;if(!c)break;}
 return 1;
}
#endif
