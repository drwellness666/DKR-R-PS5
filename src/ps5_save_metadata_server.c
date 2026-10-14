/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only backing-file metadata for the native nullfs view. Loopback only;
 * never opens file contents, follows symlinks, writes files or launches apps.
 */
#include "ps5_save_metadata_protocol.h"
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
static int receive(int fd,void*out,size_t size){char*p=out;while(size){ssize_t n=recv(fd,p,size,0);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;size-=n;}return 0;}
static int safe_metadata(const char*path,struct stat*out){char prefix[DKR_SAVE_METADATA_PATH_MAX];size_t n=strlen(path);memcpy(prefix,path,n+1);struct stat st;for(size_t i=1;i<n;i++)if(prefix[i]=='/'){prefix[i]=0;int rc=lstat(prefix,&st);prefix[i]='/';if(rc||!S_ISDIR(st.st_mode))return -1;}return lstat(path,out);}
int main(void){setvbuf(stdout,NULL,_IOLBF,0);int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return 1;int on=1;setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));struct sockaddr_in a={0};
#if defined(__FreeBSD__) || defined(__PROSPERO__) || defined(__SCE__)
 a.sin_len=sizeof(a);
#endif
 a.sin_family=AF_INET;a.sin_port=htons(DKR_SAVE_METADATA_PORT);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(bind(fd,(struct sockaddr*)&a,sizeof(a))||listen(fd,8)){close(fd);return 2;}puts("DKR-R read-only save metadata ready on loopback 9038; only own profiles; no content access");
 // Stay available across long pauses and later relaunches until the payload
 // process is stopped or the console reboots. Individual peers remain bounded.
 for(;;){struct pollfd pfd={fd,POLLIN,0};int ready=poll(&pfd,1,-1);if(ready<0&&errno==EINTR)continue;if(ready<=0)break;int peer=accept(fd,NULL,NULL);if(peer<0)continue;struct timeval t={1,0};setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));setsockopt(peer,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));struct dkrr_metadata_request request={0};struct dkrr_metadata_reply reply={.magic=0x444b5252,.status=1};
 if(!receive(peer,&request,sizeof(request))&&request.magic==0x444b5252&&memchr(request.path,0,sizeof(request.path))&&dkrr_metadata_allowed(request.path)){struct stat s;if(!safe_metadata(request.path,&s)&&S_ISREG(s.st_mode)&&s.st_uid==1&&s.st_size>=0&&s.st_size<=1048576){reply.status=0;reply.inode=s.st_ino;reply.bytes=s.st_size;reply.modified=s.st_mtime;reply.mode=s.st_mode;reply.uid=s.st_uid;reply.gid=s.st_gid;reply.links=s.st_nlink;}}
 send(peer,&reply,sizeof(reply),MSG_NOSIGNAL);close(peer);}
 close(fd);return 0;}
