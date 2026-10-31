/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include "sljitLir.h"
#include <ps5platform/exec.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static unsigned allocations,releases,live,fail_next;
struct owned {void *address;size_t bytes;} regions[128];
void *ps5_exec_allocate(size_t bytes,uintptr_t anchor){
 assert(anchor);if(fail_next){fail_next=0;return NULL;}
 size_t n=(bytes+65535)&~(size_t)65535;void *p=mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 if(p==MAP_FAILED)return NULL;if(mprotect(p,n,PROT_READ|PROT_WRITE|PROT_EXEC)){munmap(p,n);return NULL;}
 for(unsigned i=0;i<128;++i)if(!regions[i].address){regions[i]=(struct owned){p,n};++allocations;++live;return p;}
 munmap(p,n);return NULL;
}
int ps5_exec_release(void *p){for(unsigned i=0;i<128;++i)if(regions[i].address==p){assert(!munmap(p,regions[i].bytes));regions[i].address=NULL;++releases;--live;return 0;}assert(0);return -1;}
__attribute__((no_sanitize("function"))) static sljit_sw call(void*code){return ((sljit_sw(*)(void))code)();}
static struct sljit_compiler *function(int value){
 struct sljit_compiler *c=sljit_create_compiler(NULL);assert(c);
 assert(!sljit_emit_enter(c,0,SLJIT_ARGS0(W),1,0,0));
 assert(!sljit_emit_return(c,SLJIT_MOV,SLJIT_IMM,value));return c;
}
int main(void){
 for(int i=0;i<2000;++i){struct sljit_compiler *c=function(i);void*code=sljit_generate_code(c,0,NULL);assert(code&&call(code)==i);sljit_free_compiler(c);sljit_free_code(code,NULL);}
 sljit_free_unused_memory_exec();assert(allocations&&allocations==releases&&!live);
 fail_next=1;struct sljit_compiler *c=function(42);assert(!sljit_generate_code(c,0,NULL));sljit_free_compiler(c);sljit_free_unused_memory_exec();assert(!live);
 puts("PASS actual SLJIT with PS5 SDK chunk API: 2000 emitted/executed/freed functions, pooled ownership, allocator failure and no leaked executable regions; host SDK double");
}
