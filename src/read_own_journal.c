/* SPDX-License-Identifier: GPL-3.0-or-later
 * Bounded read-only snapshot of one explicitly chosen own DKR-R journal.
 * The reader drops only its own effective identity to the known game uid 1.
 * No IPC, file writes, other-process inspection or changes to another process. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_JOURNAL_DIRECTORY
#define DKR_JOURNAL_DIRECTORY "/data/dkrr/diagnostics"
#endif
#ifndef DKR_JOURNAL_NAME
#error "Build with an explicitly selected own journal basename"
#endif
#ifndef DKR_JOURNAL_UID
#define DKR_JOURNAL_UID 1
#endif
#ifndef DKR_BUILD_ID
#error "Build with the exact candidate build id"
#endif
#define DKR_EXPECTED_HEADER "DKRR_SESSION v=1 build=" DKR_BUILD_ID " pid="
#define DKR_JOURNAL_MAX_BYTES (65536U+512U)
int main(void){
 setvbuf(stdout,0,_IOLBF,0);
 printf("DKRR JOURNAL READER: own process uid=%u euid=%u\n",(unsigned)getuid(),(unsigned)geteuid());
 if(geteuid()!=DKR_JOURNAL_UID && setuid(DKR_JOURNAL_UID)!=0){perror("DKRR JOURNAL STOP: drop own reader identity");return 1;}
 if(geteuid()!=DKR_JOURNAL_UID){puts("DKRR JOURNAL STOP: reader identity mismatch");return 2;}
 const char *name=DKR_JOURNAL_NAME;
 if(strncmp(name,"run-",4)||strchr(name,'/')||strstr(name,"..")){puts("DKRR JOURNAL STOP: unexpected basename");return 3;}
 int directory=open(DKR_JOURNAL_DIRECTORY,O_RDONLY|O_NOFOLLOW|O_DIRECTORY);
 if(directory<0){perror("DKRR JOURNAL STOP: directory");return 4;}
 int fd=openat(directory,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);close(directory);
 if(fd<0){perror("DKRR JOURNAL STOP: file");return 5;}
 struct stat st={0};static char buffer[DKR_JOURNAL_MAX_BYTES+1];size_t used=0;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_uid!=DKR_JOURNAL_UID||st.st_size<1||st.st_size>DKR_JOURNAL_MAX_BYTES){printf("DKRR JOURNAL STOP: metadata uid=%u mode=%o size=%llu\n",(unsigned)st.st_uid,(unsigned)st.st_mode,(unsigned long long)st.st_size);close(fd);return 6;}
 while(used<sizeof(buffer)){ssize_t n=read(fd,buffer+used,sizeof(buffer)-used);if(n<0&&errno==EINTR)continue;if(n<0){perror("DKRR JOURNAL STOP: read");close(fd);return 7;}if(!n)break;used+=(size_t)n;}
 close(fd);
 if(used>DKR_JOURNAL_MAX_BYTES||used<strlen(DKR_EXPECTED_HEADER)||memcmp(buffer,DKR_EXPECTED_HEADER,strlen(DKR_EXPECTED_HEADER))){puts("DKRR JOURNAL STOP: size or exact build header mismatch");return 8;}
 puts("DKRR JOURNAL COPY BEGIN");
 if(fwrite(buffer,1,used,stdout)!=used)return 9;
 if(buffer[used-1]!='\n')putchar('\n');
 puts("DKRR JOURNAL COPY PASS");return 0;
}
