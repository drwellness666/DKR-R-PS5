/* SPDX-License-Identifier: GPL-3.0-or-later
 * Supplement only unknown native link counts with actual backing metadata.
 * Real stat results and admission checks stay authoritative. Failure leaves
 * nlink=0, so upstream refuses storage instead of guessing that links==1.
 */
#include "ps5_save_metadata_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
int __real_stat(const char*,struct stat*);
static atomic_uint debug_count;
static int transfer(int fd,void*buffer,size_t bytes,int sending){char*p=buffer;while(bytes){ssize_t n=sending?send(fd,p,bytes,MSG_NOSIGNAL):recv(fd,p,bytes,0);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=n;}return 0;}
int __wrap_stat(const char*path,struct stat*out){int rc=__real_stat(path,out),saved=errno;if(rc||!S_ISREG(out->st_mode)||out->st_nlink||(out->st_uid!=0 && out->st_uid!=1)||strnlen(path,DKR_SAVE_METADATA_PATH_MAX)==DKR_SAVE_METADATA_PATH_MAX||!dkrr_metadata_allowed(path))return rc;
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0){errno=saved;return rc;}struct timeval timeout={0,250000};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));struct sockaddr_in a={0};
#if defined(__FreeBSD__) || defined(__PROSPERO__) || defined(__SCE__)
 a.sin_len=sizeof(a);
#endif
 a.sin_family=AF_INET;a.sin_port=htons(DKR_SAVE_METADATA_PORT);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);struct dkrr_metadata_request request={.magic=0x444b5252};memcpy(request.path,path,strlen(path)+1);struct dkrr_metadata_reply reply={0};
 int connected=connect(fd,(struct sockaddr*)&a,sizeof(a));int sent=connected?-1:transfer(fd,&request,sizeof(request),1);int got=sent?-1:transfer(fd,&reply,sizeof(reply),0);int network_errno=errno;
 if(atomic_fetch_add(&debug_count,1)<12)fprintf(stderr,"[ps5][save-stat] net=%d/%d/%d errno=%d reply=%u native=[%llu,%llu,%llu,%o,%u] backing=[%llu,%llu,%llu,%o,%u] links=%u\n",connected,sent,got,network_errno,reply.status,(unsigned long long)out->st_ino,(unsigned long long)out->st_size,(unsigned long long)out->st_mtime,(unsigned)out->st_mode,(unsigned)out->st_uid,(unsigned long long)reply.inode,(unsigned long long)reply.bytes,(unsigned long long)reply.modified,reply.mode,reply.uid,reply.links);
 if(!connected&&!sent&&!got&&reply.magic==request.magic&&!reply.status&&reply.inode==(uint64_t)out->st_ino&&reply.bytes==(uint64_t)out->st_size&&reply.modified==(uint64_t)out->st_mtime&&reply.mode==(uint32_t)out->st_mode&&reply.uid==1){struct stat again;if(!__real_stat(path,&again)&&again.st_ino==out->st_ino&&again.st_size==out->st_size&&again.st_mtime==out->st_mtime&&again.st_mode==out->st_mode&&again.st_uid==out->st_uid)out->st_nlink=reply.links;}
 close(fd);errno=saved;return rc;}
