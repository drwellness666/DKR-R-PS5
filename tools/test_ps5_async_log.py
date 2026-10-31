#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prove diagnostic producers remain nonblocking under a blocked native writer."""
import os,subprocess,tempfile
from pathlib import Path
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dkrr-async-log-') as temp:
 t=Path(temp);source=r'''#define _GNU_SOURCE 1
#include "ps5_async_log.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static atomic_int block_writer,entered,release_writer;
static pthread_t main_thread;
ssize_t __real_write(int,const void*,size_t);
ssize_t __wrap_write(int fd,const void*bytes,size_t count){if(atomic_load(&block_writer)&&!pthread_equal(main_thread,pthread_self())){atomic_store(&entered,1);while(!atomic_load(&release_writer))usleep(100);}
 return __real_write(fd,bytes,count);}
static void*producer(void*opaque){for(unsigned i=0;i<10000;++i)fprintf(stderr,"producer %ld / %u\n",(long)opaque,i);return NULL;}
int main(int argc,char**argv){(void)argc;main_thread=pthread_self();char path[1024],other[1024],diagnostic[256];snprintf(path,sizeof(path),"%s/runtime.log",argv[1]);snprintf(other,sizeof(other),"%s/save.bin",argv[1]);int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0);
 assert(!dkrr_async_log_start(-1));int ends[2];assert(!pipe(ends));assert(!dkrr_async_log_start(ends[1]));close(ends[0]);close(ends[1]);assert(dkrr_async_log_start(fd));assert(dkrr_async_log_start(fd));
 FILE*file=fopen(other,"wb");assert(file);assert(fwrite("\0\1\2",1,3,file)==3);assert(fputs("save",file)>=0);assert(fprintf(file,"%d",123)==3);fclose(file);
 atomic_store(&block_writer,1);fprintf(stderr,"first record\n");for(unsigned tries=0;tries<10000&&!atomic_load(&entered);++tries)usleep(100);assert(atomic_load(&entered));
 pthread_t threads[4];for(long i=0;i<4;++i)assert(!pthread_create(&threads[i],NULL,producer,(void*)i));for(unsigned i=0;i<4;++i)pthread_join(threads[i],NULL);
 assert(!atomic_load(&release_writer)); // 40000 producer calls completed while disk worker is still blocked.
 assert(fwrite("",0,7,stderr)==0);assert(fwrite("",(size_t)-1,2,stderr)==0&&errno==EOVERFLOW);
 assert(dkrr_async_log_diagnostic(diagnostic,sizeof(diagnostic))>0);assert(strstr(diagnostic,"async=1"));assert(!strstr(diagnostic,"dropped=0 "));
 atomic_store(&release_writer,1);dkrr_async_log_stop();dkrr_async_log_stop();assert(dkrr_async_log_diagnostic(diagnostic,sizeof(diagnostic))>0&&strstr(diagnostic,"async=0"));
 file=fopen(other,"rb");assert(file);char data[16]={0};assert(fread(data,1,sizeof(data),file)==10);fclose(file);assert(!memcmp(data,"\0\1\2save123",10));
 file=fopen(path,"rb");assert(file);char log[200000];size_t length=fread(log,1,sizeof(log)-1,file);fclose(file);log[length]=0;assert(strstr(log,"first record"));assert(strstr(log,"dropped="));assert(length<150000);
 assert(fcntl(fd,F_GETFD)>=0);assert(dkrr_async_log_start(fd));puts("restarted writer");dkrr_async_log_stop();assert(fcntl(fd,F_GETFD)>=0);close(fd);
 puts("PASS native diagnostic spool: 40000 calls across 4 producers complete with disk blocked; bounded drops, writer drain/restart, original binary/save FILE I/O and overflow semantics");}
''';(t/'test.c').write_text(source)
 wraps=['fprintf','vfprintf','printf','vprintf','fputs','puts','fwrite','write']
 subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-I'+str(r/'src'),str(r/'src/ps5_async_log.c'),str(t/'test.c'),*['-Wl,--wrap='+n for n in wraps],'-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test'),str(t)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
