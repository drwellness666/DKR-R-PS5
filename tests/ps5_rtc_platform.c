/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
static unsigned calls;
static int fail;
static int device_allowed,device_regular,device_eof,device_error,interrupt_read;
static unsigned opens,reads,closes;
static int kernel_allowed,kernel_error,kernel_eof;
static unsigned kernel_calls;
int sysctl(const int* mib,unsigned count,void* out,size_t* size,const void* input,size_t bytes){
 assert(count==2&&mib[0]==1&&mib[1]==37&&!input&&!bytes&&*size<=256);++kernel_calls;
 if(!kernel_allowed || (kernel_error&&kernel_calls>1))return -1;
 if(kernel_eof){*size=0;return 0;}if(*size>17)*size=17;memset(out,0x6d,*size);return 0;
}
int __wrap_open(const char *path,int flags,...){assert(!strcmp(path,"/dev/urandom"));assert((flags&(O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC))==(O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));++opens;return device_allowed?73:-1;}
int __wrap_fstat(int fd,struct stat* info){assert(fd==73);memset(info,0,sizeof(*info));info->st_mode=device_regular?S_IFREG|0600:S_IFCHR|0600;return 0;}
ssize_t __wrap_read(int fd,void *out,size_t size){assert(fd==73&&size<=256);++reads;if(interrupt_read){interrupt_read=0;errno=EINTR;return -1;}if(device_eof)return 0;if(device_error&&reads>1){errno=EIO;return -1;}size_t n=size>17?17:size;memset(out,0x5b,n);return (ssize_t)n;}
int __wrap_close(int fd){assert(fd==73);++closes;return 0;}
int sceRandomGetRandomNumber(void *out,size_t size){assert(size>0&&size<=256);++calls;if(fail)return -1;memset(out,0xa7,size);return 0;}
int mbedtls_hardware_poll(void *,unsigned char *,size_t,size_t *);
void juice_random(void *,size_t);
void juice_random_str64(char *,size_t);
uint32_t juice_rand32(void);
uint64_t juice_rand64(void);
void __wrap_read_random(void *,size_t);
long __wrap_syscall(long,...);
int timingsafe_bcmp(const void *,const void *,size_t);
int pthread_getthreadid_np(void){return 77;}
int dkrr_rtc_entropy_available(void);
int dkrr_secure_random(void*,size_t);
int main(int argc,char **argv){
 unsigned char bytes[1025];size_t n=99;
 if(argc>1){
  fail=1;
  if(!strcmp(argv[1],"failure")){struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit);juice_random(bytes,sizeof(bytes));return 99;}
  if(!strncmp(argv[1],"kernel",6)){
   kernel_allowed=1;assert(dkrr_rtc_entropy_available()&&opens==0);kernel_calls=0;
   kernel_error=!strcmp(argv[1],"kernel-error");kernel_eof=!strcmp(argv[1],"kernel-eof");
   memset(bytes,1,sizeof(bytes));int rc=mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n);
   assert((rc==0)==(!kernel_error&&!kernel_eof));assert(n==(rc?0:sizeof(bytes)));
   for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==(rc?0:0x6d));
   assert(opens==0);puts("PASS OS kernel entropy: exact MIB, partial blocks, error/zero-length refusal and complete output erasure");return 0;
  }
  device_allowed=1;device_regular=!strcmp(argv[1],"regular");device_eof=!strcmp(argv[1],"eof");interrupt_read=1;
  if(device_regular||device_eof){assert(!dkrr_rtc_entropy_available()&&opens==1&&closes==1);memset(bytes,1,sizeof(bytes));assert(mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n)!=0&&n==0);for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0);puts("PASS non-device/EOF entropy source refused");return 0;}
  assert(dkrr_rtc_entropy_available()&&opens==1&&calls==1);unsigned before=reads;
  for(unsigned i=0;i<100;++i)assert(dkrr_rtc_entropy_available());assert(reads==before&&opens==1);
  reads=0;device_error=!strcmp(argv[1],"device-error");memset(bytes,0,sizeof(bytes));
  if(device_error){assert(mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n)!=0&&n==0);for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0);}
  else {assert(!mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n)&&n==sizeof(bytes));for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0x5b);assert(!dkrr_secure_random(bytes,sizeof(bytes)));}
  assert(calls==1&&opens==1);puts("PASS cached native OS random device, bounded partial reads/EINTR and erasure on failure; no PRNG fallback");return 0;
 }
 assert(dkrr_rtc_entropy_available()&&calls==1&&opens==0);calls=0;
 memset(bytes,0,sizeof(bytes));assert(!mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n)&&n==sizeof(bytes)&&calls==5);
 for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0xa7);
 calls=0;juice_random(bytes,513);assert(calls==3);
 calls=0;juice_random_str64((char *)bytes,33);assert(calls==32&&bytes[32]==0);
 for(unsigned i=0;i<32;++i)assert(bytes[i]=='n');
 assert(juice_rand32()==UINT32_C(0xa7a7a7a7));assert(juice_rand64()==UINT64_C(0xa7a7a7a7a7a7a7a7));assert(calls==34);
 juice_random_str64(NULL,0);bytes[0]=99;juice_random_str64((char *)bytes,1);assert(bytes[0]==0&&calls==34);
 __wrap_read_random(bytes,1025);assert(calls==39);
 for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0xa7);
 long thread=0;assert(__wrap_syscall(432,&thread)==0&&thread==77);
 assert(__wrap_syscall(432,NULL)==-1&&errno==EFAULT);
 assert(__wrap_syscall(0,&thread)==-1&&errno==ENOSYS&&thread==77);
 unsigned char same[sizeof(bytes)];memcpy(same,bytes,sizeof(bytes));assert(!timingsafe_bcmp(bytes,same,sizeof(bytes)));
 for(unsigned i=0;i<sizeof(bytes);++i){same[i]^=1;assert(timingsafe_bcmp(bytes,same,sizeof(bytes)));same[i]^=1;}
 assert(!timingsafe_bcmp(NULL,NULL,0));
 fail=1;n=77;assert(mbedtls_hardware_poll(NULL,bytes,sizeof(bytes),&n)==-0x003c&&n==0);
 for(unsigned i=0;i<sizeof(bytes);++i)assert(bytes[i]==0);
 assert(mbedtls_hardware_poll(NULL,NULL,1,&n)==-0x003c&&n==0);
 assert(mbedtls_hardware_poll(NULL,NULL,0,NULL)==-0x003c);
 assert(!mbedtls_hardware_poll(NULL,NULL,0,&n)&&n==0);
 puts("PASS native RTC entropy: secure-service-only 256-byte chunks, exact length, error and partial-output erasure; no clock/PRNG fallback");
}
