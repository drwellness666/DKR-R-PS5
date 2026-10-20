/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real adapter with a modeled nullfs stat and socket transport; no console or files touched.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/resource.h>
#include "ps5_save_metadata_protocol.h"
static unsigned sockets,calls,reply_kind,links=1;static struct stat metadata;
int __wrap_stat(const char*,struct stat*);
int __real_stat(const char*path,struct stat*out){(void)path;*out=metadata;calls++;if(reply_kind==8&&calls>1)out->st_ino++;errno=17;return 0;}
int test_socket(int a,int b,int c){(void)a;(void)b;(void)c;sockets++;return 22;}
int test_setsockopt(int a,int b,int c,const void*d,socklen_t e){(void)a;(void)b;(void)c;(void)d;(void)e;return 0;}
int test_connect(int a,const struct sockaddr*b,socklen_t c){(void)a;(void)b;(void)c;if(reply_kind==1){errno=ECONNREFUSED;return -1;}return 0;}
ssize_t test_send(int fd,const void*p,size_t n,int flags){(void)fd;(void)flags;const struct dkrr_metadata_request*r=p;assert(n==sizeof(*r)&&r->magic==0x444b5252&&dkrr_metadata_allowed(r->path));return n;}
ssize_t test_recv(int fd,void*p,size_t n,int flags){(void)fd;(void)flags;struct dkrr_metadata_reply r={.magic=0x444b5252,.inode=metadata.st_ino,.bytes=metadata.st_size,.modified=metadata.st_mtime,.mode=metadata.st_mode,.uid=1,.gid=metadata.st_gid,.links=links};assert(n==sizeof(r));if(reply_kind==2)r.magic=0;if(reply_kind==3)r.status=1;if(reply_kind==4)r.inode++;if(reply_kind==5)r.bytes++;if(reply_kind==6)r.modified++;if(reply_kind==7)r.uid++;memcpy(p,&r,sizeof(r));return sizeof(r);}
int test_close(int n){assert(n==22);return 0;}
int main(void){
 const char* paths[]={"saves/custom-records-v1.bin","save-migration-v1/manifest.json","save-migration-v1/images/a.bin","save-transactions/20261020/manifest.json","pending-save-transaction/manifest.json","save-recovery/a.bin","pending-development-seed.bin"};
 for(unsigned i=0;i<sizeof(paths)/sizeof(paths[0]);++i){char own[512];snprintf(own,sizeof(own),"/data/dkrr/profile-us-v77/%s",paths[i]);assert(dkrr_metadata_allowed(own));}
 assert(!dkrr_metadata_allowed("/data/dkrr/profile-us-v77/save-recovery/../foreign"));
 assert(!dkrr_metadata_allowed("/data/dkrr/preferences/save-recovery/file.bin"));
struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit);assert(sizeof(struct dkrr_metadata_request)==516&&sizeof(struct dkrr_metadata_reply)==48);const char*owned="/data/dkrr/profile-us-v77/saves/dkr.us.v77.bin";
 assert(dkrr_metadata_allowed(owned));assert(dkrr_metadata_allowed("/data/dkrr/profile-us-v80/controller-pak-1.mpk"));
 assert(dkrr_metadata_allowed("/data/dkrr/profile-us-v77/save-backups/adventure-20261013.bin.lock"));
 assert(dkrr_metadata_allowed("/data/dkrr/profile-us-v80/save-history/controller-pak-1.mpk/previous.mpk"));
 assert(!dkrr_metadata_allowed("/data/dkrr/profile-us-v77/save-backups/../roms/a"));
 assert(!dkrr_metadata_allowed("/data/dkrr/profile-us-v77/save-history/"));
 for(const char**p=(const char*[]){"/data/dkrr/profile-us-v77/saves/../roms/a","/data/dkrr/profile-us-v77/saves//a","/data/dkrr/profile-us-v77/roms/a","/data/foreign/saves/a","/data/dkrr/profile-us-v770/saves/a",NULL};*p;p++)assert(!dkrr_metadata_allowed(*p));
 metadata.st_mode=S_IFREG|0600;metadata.st_uid=1;metadata.st_ino=5234;metadata.st_size=2048;metadata.st_mtime=1000;
 for(reply_kind=0;reply_kind<=8;reply_kind++){calls=sockets=0;struct stat out;assert(__wrap_stat(owned,&out)==0);assert(out.st_nlink==(reply_kind?0:1));assert(errno==17);}
 reply_kind=0;links=2;calls=0;struct stat out;__wrap_stat(owned,&out);assert(out.st_nlink==2); // Alias evidence remains an alias, never forced to one.
 sockets=0;__wrap_stat("/data/foreign/saves/a",&out);assert(!sockets&&out.st_nlink==0);
 metadata.st_nlink=3;sockets=0;__wrap_stat(owned,&out);assert(!sockets&&out.st_nlink==3);
 metadata.st_nlink=0;metadata.st_uid=0;calls=0;__wrap_stat(owned,&out);assert(out.st_nlink==2);
 metadata.st_uid=5;sockets=0;__wrap_stat(owned,&out);assert(!sockets&&out.st_nlink==0);
 puts("PASS: actual save stat adapter, path containment, exact protocol layout, mismatch/race/refusal handling, original known counts and real alias preservation");}
