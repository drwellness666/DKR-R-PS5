/* SPDX-License-Identifier: GPL-3.0-or-later
 * Fixed-size, in-process diagnostic spool. Rendering never waits for file I/O.
 * Link wraps only the standard output functions; other FILE streams keep
 * their normal libc semantics, including package data and save files.
 */
#define _POSIX_C_SOURCE 200809L
#include "ps5_async_log.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { SLOTS=64, RECORD=2048, BATCH=8, LOG_LIMIT=4*1024*1024 };
static struct {size_t bytes;char text[RECORD];} queue[SLOTS];
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static unsigned head,tail,count;
static int fd=-1,stopping;
static atomic_int active;
static atomic_int start_error,start_phase;
static atomic_ullong dropped,records,io_errors;

int __real_vfprintf(FILE*,const char*,va_list);
int __real_fputs(const char*,FILE*);
size_t __real_fwrite(const void*,size_t,size_t,FILE*);
int __real_vprintf(const char*,va_list);
int __real_puts(const char*);

static void enqueue(const char *text,size_t bytes) {
    if(!atomic_load_explicit(&active,memory_order_acquire))return;
    if(pthread_mutex_trylock(&lock)){atomic_fetch_add(&dropped,1);return;}
    if(stopping||count==SLOTS){atomic_fetch_add(&dropped,1);pthread_mutex_unlock(&lock);return;}
    if(bytes>RECORD)bytes=RECORD;
    memcpy(queue[tail].text,text,bytes);queue[tail].bytes=bytes;
    tail=(tail+1)%SLOTS;++count;atomic_fetch_add(&records,1);
    pthread_cond_signal(&changed);pthread_mutex_unlock(&lock);
}
static int formatted(const char *format,va_list args) {
    char record[RECORD];const int length=vsnprintf(record,sizeof(record),format,args);
    if(length<0)return length;
    const size_t bytes=(size_t)length<sizeof(record)?(size_t)length:sizeof(record)-1;
    if((size_t)length>=sizeof(record)){
        static const char marker[]=" [log record truncated]\n";
        memcpy(record+bytes-(sizeof(marker)-1),marker,sizeof(marker)-1);
    }
    enqueue(record,bytes);return length;
}
static int captured(FILE *stream) {
    return atomic_load_explicit(&active,memory_order_acquire)&&(stream==stderr||stream==stdout);
}
int __wrap_vfprintf(FILE *stream,const char *format,va_list args) {
    return captured(stream)?formatted(format,args):__real_vfprintf(stream,format,args);
}
int __wrap_fprintf(FILE *stream,const char *format,...) {
    va_list args;va_start(args,format);const int result=__wrap_vfprintf(stream,format,args);va_end(args);return result;
}
int __wrap_vprintf(const char *format,va_list args) {
    return captured(stdout)?formatted(format,args):__real_vprintf(format,args);
}
int __wrap_printf(const char *format,...) {
    va_list args;va_start(args,format);const int result=__wrap_vprintf(format,args);va_end(args);return result;
}
int __wrap_fputs(const char *text,FILE *stream) {
    if(!captured(stream))return __real_fputs(text,stream);
    const size_t bytes=strnlen(text,RECORD);enqueue(text,bytes);return 0;
}
int __wrap_puts(const char *text) {
    if(!captured(stdout))return __real_puts(text);
    char record[RECORD];const size_t bytes=strnlen(text,RECORD-1);memcpy(record,text,bytes);record[bytes]='\n';enqueue(record,bytes+1);return 0;
}
size_t __wrap_fwrite(const void *data,size_t size,size_t items,FILE *stream) {
    if(!captured(stream))return __real_fwrite(data,size,items,stream);
    if(!size||!items)return 0;
    if(size&&items>SIZE_MAX/size){errno=EOVERFLOW;return 0;}
    const size_t bytes=size*items;if(bytes)enqueue(data,bytes);return items;
}
static void *drain(void *unused) {
    (void)unused;char batch[BATCH*RECORD];size_t written=0;unsigned long long reported_dropped=0;
    for(;;){
        pthread_mutex_lock(&lock);
        while(!count&&!stopping)pthread_cond_wait(&changed,&lock);
        if(!count&&stopping){pthread_mutex_unlock(&lock);break;}
        size_t used=0;unsigned n=0;
        while(count&&n++<BATCH){memcpy(batch+used,queue[head].text,queue[head].bytes);used+=queue[head].bytes;head=(head+1)%SLOTS;--count;}
        pthread_mutex_unlock(&lock);
        const unsigned long long lost=atomic_load(&dropped);
        if(lost!=reported_dropped&&used+96<sizeof(batch)){
            const int bytes=snprintf(batch+used,sizeof(batch)-used,"\n[ps5][log] dropped=%llu (bounded diagnostic queue)\n",lost);
            if(bytes>0)used+=(size_t)bytes;
            reported_dropped=lost;
        }
        if(written>=LOG_LIMIT)continue;
        if(used>LOG_LIMIT-written)used=LOG_LIMIT-written;
        for(size_t at=0;at<used;){
            const ssize_t bytes=write(fd,batch+at,used-at);
            if(bytes<0&&errno==EINTR)continue;
            if(bytes<=0){atomic_fetch_add(&io_errors,1);written=LOG_LIMIT;break;}
            at+=(size_t)bytes;written+=(size_t)bytes;
        }
    }
    return NULL;
}
int dkrr_async_log_start(int descriptor) {
    if(atomic_load(&active))return 1;
    atomic_store(&start_phase,1);atomic_store(&start_error,0);
    if(descriptor<0){atomic_store(&start_error,EBADF);errno=EBADF;return 0;}
    struct stat info={0};
    if(fstat(descriptor,&info)){atomic_store(&start_error,errno);return 0;}
    if(!S_ISREG(info.st_mode)){atomic_store(&start_error,EINVAL);errno=EINVAL;return 0;}
    // ConfigurePersistentRuntimeLog already opened this exact owned file.
    // Reopening by path adds a redundant native permission/identity check
    // and gives stdio and the writer separate offsets. Share its existing
    // descriptor, as the SDK recommends when dup/dup2 are refused to titles.
    fd=descriptor;head=tail=count=0;stopping=0;atomic_store(&start_phase,2);
    atomic_store(&dropped,0);atomic_store(&records,0);atomic_store(&io_errors,0);
    const int error=pthread_create(&worker,NULL,drain,NULL);
    if(error){fd=-1;atomic_store(&start_error,error);errno=error;return 0;}
    atomic_store(&start_phase,0);
    atomic_store_explicit(&active,1,memory_order_release);return 1;
}
void dkrr_async_log_stop(void) {
    if(!atomic_load(&active))return;
    pthread_mutex_lock(&lock);stopping=1;pthread_cond_signal(&changed);pthread_mutex_unlock(&lock);
    pthread_join(worker,NULL);
    atomic_store_explicit(&active,0,memory_order_release);fd=-1;
}
int dkrr_async_log_diagnostic(char *out,unsigned bytes) {
    return snprintf(out,bytes,"DKRR_LOG v=1 async=%u records=%llu dropped=%llu io-errors=%llu start-phase=%d start-error=%d\n",
        (unsigned)atomic_load(&active),(unsigned long long)atomic_load(&records),
        (unsigned long long)atomic_load(&dropped),(unsigned long long)atomic_load(&io_errors),atomic_load(&start_phase),atomic_load(&start_error));
}
