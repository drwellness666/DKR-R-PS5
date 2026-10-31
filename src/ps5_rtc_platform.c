/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__PROSPERO__)
#include <sys/syscall.h>
#include <sys/sysctl.h>
#define DKRR_RTC_THR_SELF SYS_thr_self
#else
#define DKRR_RTC_THR_SELF 432
#define CTL_KERN 1
#define KERN_ARND 37
extern int sysctl(const int*,unsigned,void*,size_t*,const void*,size_t);
#endif
extern int sceRandomGetRandomNumber(void *,size_t);
#pragma weak sceRandomGetRandomNumber
extern int pthread_getthreadid_np(void);
static pthread_once_t entropy_once=PTHREAD_ONCE_INIT;
static int entropy_source,random_device=-1;
/* The public SDK's arc4random.c obtains its seed through this FreeBSD
 * kernel interface. Use only the OS bytes, without its PRNG/fallback path. */
static int kernel_bytes(void *out,size_t bytes) {
    const int mib[2]={CTL_KERN,KERN_ARND};unsigned char *p=out;
    while(bytes){size_t n=bytes>256?256:bytes;
        if(sysctl(mib,2,p,&n,NULL,0)!=0 || !n || n>bytes || n>256)return -1;
        p+=n;bytes-=n;
    }
    return 0;
}
static int device_bytes(void *out,size_t bytes) {
    unsigned char *p=out;
    while(bytes){
        const size_t chunk=bytes>256?256:bytes;
        ssize_t n=read(random_device,p,chunk);
        if(n<0&&errno==EINTR)continue;
        if(n<=0 || (size_t)n>chunk)return -1;
        p+=n;bytes-=(size_t)n;
    }
    return 0;
}
static void initialize_entropy(void) {
    unsigned char probe[32]={0};
    int (*volatile native_random)(void *,size_t)=sceRandomGetRandomNumber;
    if(native_random && native_random(probe,sizeof(probe))==0)entropy_source=1;
    else if(kernel_bytes(probe,sizeof(probe))==0)entropy_source=3;
    else {
        random_device=open("/dev/urandom",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        struct stat info={0};
        if(random_device>=0 && fstat(random_device,&info)==0 && S_ISCHR(info.st_mode) &&
           device_bytes(probe,sizeof(probe))==0)entropy_source=2;
        else if(random_device>=0){close(random_device);random_device=-1;}
    }
    volatile unsigned char *clear=probe;
    for(size_t i=0;i<sizeof(probe);++i)clear[i]=0;
}
int dkrr_rtc_entropy_available(void) {
    return pthread_once(&entropy_once,initialize_entropy)==0 && entropy_source!=0;
}
const char* dkrr_rtc_entropy_source(void) {
    if(!dkrr_rtc_entropy_available())return "unavailable";
    return entropy_source==1?"native random module":entropy_source==3?"kernel random service":"system random device";
}
static int secure_bytes(void *out,size_t bytes) {
    if(!out&&bytes)return -1;
    if(!bytes)return 0;
    if(!dkrr_rtc_entropy_available())return -1;
    if(entropy_source==2)return device_bytes(out,bytes);
    if(entropy_source==3)return kernel_bytes(out,bytes);
    int (*volatile native_random)(void *,size_t)=sceRandomGetRandomNumber;
    if(!native_random)return -1;
    unsigned char *p=out;
    while(bytes){size_t chunk=bytes>256?256:bytes;
        if(native_random(p,chunk)!=0)return -1;
        p+=chunk;bytes-=chunk;
    }
    return 0;
}
/* Shared with the Store's OAuth verifier. Never substitute a timestamp PRNG. */
int dkrr_secure_random(void *out,size_t bytes) {
    if(secure_bytes(out,bytes)){
        if(out)memset(out,0,bytes);
        return -1;
    }
    return 0;
}
int mbedtls_hardware_poll(void *context,unsigned char *out,size_t bytes,size_t *written) {
    (void)context;if(!written)return -0x003c;*written=0;
    if(secure_bytes(out,bytes)){if(out)memset(out,0,bytes);return -0x003c;}
    *written=bytes;return 0;
}
// libjuice's generic non-Linux fallback seeds random() from the clock.
// ICE credentials must use the native secure RNG, with no weak fallback.
void juice_random(void *out,size_t bytes) {
    if(secure_bytes(out,bytes)){
        if(out)memset(out,0,bytes);
        fputs("[ps5][online] native secure RNG failed; refusing insecure ICE credentials\n",stderr);
        abort();
    }
}
void juice_random_str64(char *out,size_t size) {
    static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if(!size)return;
    if(!out)abort();
    for(size_t i=0;i+1<size;++i){unsigned char value;juice_random(&value,1);out[i]=alphabet[value&63U];}
    out[size-1]='\0';
}
uint32_t juice_rand32(void){uint32_t value;juice_random(&value,sizeof(value));return value;}
uint64_t juice_rand64(void){uint64_t value;juice_random(&value,sizeof(value));return value;}
/* SCTP's read_random is only called from other translation units, so the
 * linker wrapper also keeps its cookie/authentication secrets on this RNG. */
void __wrap_read_random(void *out,size_t size){juice_random(out,size);}

/* PLog's FreeBSD thread-label path uses syscall(SYS_thr_self). Route that one
 * operation to the existing native SDK export; never issue a raw syscall. */
long __wrap_syscall(long number,...) {
    if(number!=DKRR_RTC_THR_SELF){errno=ENOSYS;return -1;}
    va_list arguments;va_start(arguments,number);long *thread=va_arg(arguments,long *);va_end(arguments);
    if(!thread){errno=EFAULT;return -1;}
    const int id=pthread_getthreadid_np();
    if(id<0){errno=EIO;return -1;}
    *thread=id;return 0;
}

/* SCTP cookie/HMAC comparison must examine every byte, including after the
 * first mismatch. Volatile reads prevent replacement with early-exit memcmp. */
int timingsafe_bcmp(const void *left,const void *right,size_t size) {
    const volatile unsigned char *a=left,*b=right;unsigned difference=0;
    for(size_t i=0;i<size;++i)difference|=a[i]^b[i];
    return difference!=0;
}
