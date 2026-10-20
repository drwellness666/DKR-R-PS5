/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only check of only our proposed native title and ROM locations.
 * No installs, mounts, launches, process termination or privilege changes.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
int sceUserServiceInitialize(void *);
int sceSystemServiceGetAppId(const char *);
static void inspect(const char *path) {
    struct stat info;
    if (lstat(path,&info)==0)
        printf("preflight: exists path=%s type=%s mode=%o bytes=%llu\n",path,
               S_ISREG(info.st_mode)?"file":S_ISDIR(info.st_mode)?"directory":"other",
               (unsigned)(info.st_mode&0777),(unsigned long long)info.st_size);
    else printf("preflight: absent-or-inaccessible path=%s errno=%d (%s)\n",path,errno,strerror(errno));
}
int main(void) {
    setvbuf(stdout,NULL,_IOLBF,0);
    puts("DKR-R 1.0.7 GAME PREFLIGHT: READ ONLY");
    int init=sceUserServiceInitialize(NULL);
    printf("preflight: user-service rc=0x%08x\n",(unsigned)init);
    int app=sceSystemServiceGetAppId("PPSA94678");
    printf("preflight: proposed-title lookup rc=0x%08x\n",(unsigned)app);
    const char *paths[]={"/data/homebrew/PPSA94678","/user/app/PPSA94678","/user/appmeta/PPSA94678",
        "/data/homebrew/PPSA94677/eboot.bin","/data/dkrr/roms/dkr.us.v77.z64","/data/dkrr/roms/dkr.us.v80.z64"};
    for(unsigned i=0;i<sizeof(paths)/sizeof(paths[0]);i++)inspect(paths[i]);
    puts("PREFLIGHT DONE: no console files or applications changed");
    return 0;
}
