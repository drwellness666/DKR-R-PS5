/* SPDX-License-Identifier: GPL-3.0-or-later
 * Universal, ROM-free payload core. No compiler, PC build receipt, kernel
 * patching, app.db edit, network download or resident helper on the console.
 */
#include "dkrr_install.h"
#include "../src/ps5_controller_audio_bank.h"
#include "../src/ps5_storage_budget.h"
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "release_files.h"
#ifndef DKRR_INSTALL_ROOT
#define DKRR_INSTALL_ROOT "/data"
#endif
#ifndef DKRR_USB_ROOT
#define DKRR_USB_ROOT "/mnt"
#endif
#ifndef DKRR_SYSTEM_USER_ROOT
#define DKRR_SYSTEM_USER_ROOT "/user"
#endif
#ifndef DKRR_UPDATE_ONLY
#define DKRR_UPDATE_ONLY 0
#endif
#ifndef DKRR_NETWORK_UPDATE
#define DKRR_NETWORK_UPDATE 0
#endif
#if DKRR_NETWORK_UPDATE
#include "network_gate.h"
#endif
static char data[DKRR_PATH],owned[DKRR_PATH],install[DKRR_PATH],live[DKRR_PATH],stage[DKRR_PATH],backup[DKRR_PATH],journal[DKRR_PATH];
#ifndef DKRR_HOST_TEST
extern int sceKernelSendNotificationRequest(int,void*,size_t,int);
#endif
static void status(const char *message) {
 printf("DKRR INSTALL: %s\n",message);
#ifndef DKRR_HOST_TEST
 struct { char reserved[45];char message[3075]; } request={{0},{0}};
 snprintf(request.message,sizeof(request.message),"DKR-R: %s",message);
 sceKernelSendNotificationRequest(0,&request,sizeof(request),0);
#endif
}
static int join(char *out,const char *a,const char *b){int n=snprintf(out,DKRR_PATH,"%s/%s",a,b);return n<0||n>=DKRR_PATH?-1:0;}
static int title_identity_available(void) {
 const struct release_file *metadata=NULL;
 for(size_t i=0;i<sizeof(release_files)/sizeof(release_files[0]);++i)if(!strcmp(release_files[i].path,"sce_sys/param.json"))metadata=&release_files[i];
 if(!metadata)return 0;
 const char *names[]={"app/PPSA94678/sce_sys/param.json","appmeta/PPSA94678/param.json"};
 for(unsigned i=0;i<2;++i){char path[DKRR_PATH],hex[41];unsigned char digest[20];struct stat st;
  if(join(path,DKRR_SYSTEM_USER_ROOT,names[i])<0)return 0;
  if(lstat(path,&st)<0){if(errno==ENOENT)continue;return 0;}
  // Read-only cache inspection permits framework-owned parent indirection.
  // Neither this file nor its target is ever changed by the payload.
  int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return 0;
  int ok=fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&(uint64_t)st.st_size==(uint64_t)(metadata->end-metadata->start)&&dkrr_sha1_fd(fd,digest)==0;close(fd);
  if(!ok)return 0;dkrr_hex(digest,hex);if(strcmp(hex,metadata->sha1))return 0;
 }return 1;
}
static int exists_dir(const char *p){struct stat st;if(lstat(p,&st)<0)return errno==ENOENT?0:-1;return S_ISDIR(st.st_mode)?1:-1;}
/* The app owns update-client-1 with mode 0700. PS5 does not let a payload's
 * euid 0 inspect that directory as if it owned it. Read only this state under
 * the app identity, then restore the installer identity on every result. */
static int app_update_pending(const char *path) {
#ifdef DKRR_HOST_TEST
 const uid_t app_uid=getuid();
#else
 const uid_t app_uid=1;
#endif
 const uid_t previous=geteuid();
 if(previous!=app_uid&&seteuid(app_uid)<0)return -1;
 const int found=exists_dir(path),saved=errno;
 if(previous!=app_uid&&seteuid(previous)<0)return -1;
 errno=saved;return found;
}
static int own_manifest(const char *app,struct dkrr_manifest *m) {
 char p[DKRR_PATH];if(join(p,app,"dkrr-install.manifest")<0)return -1;
 return dkrr_manifest_read(p,m)==0&&dkrr_manifest_check(app,m,1)==0?0:-1;
}
static int known_at(const char *app,struct dkrr_manifest *m) {
 char path[DKRR_PATH];struct stat st;if(join(path,app,"dkrr-install.manifest")<0)return -1;
 if(lstat(path,&st)==0)return own_manifest(app,m);
 if(errno!=ENOENT)return -1;
 for(size_t i=0;i<sizeof(legacy_manifests)/sizeof(legacy_manifests[0]);++i){
  const struct dkrr_manifest *candidate=&legacy_manifests[i];
  if(candidate->count&&dkrr_manifest_check(app,candidate,1)==0){*m=*candidate;return 0;}
 }return -1;
}
static int same_manifest(const struct dkrr_manifest *a,const struct dkrr_manifest *b){
 if(strcmp(a->build,b->build)||a->sequence!=b->sequence||a->count!=b->count)return 0;
 for(size_t i=0;i<a->count;++i)if(strcmp(a->files[i].path,b->files[i].path)||strcmp(a->files[i].sha1,b->files[i].sha1)||a->files[i].size!=b->files[i].size||a->files[i].mode!=b->files[i].mode)return 0;
 return 1;
}
/* Journal contains the complete old manifest. A power loss between the two
 * directory renames is repaired by rerunning either compatible payload. */
static int recover(void) {
 int state=dkrr_regular(journal,1);if(state<0)return -1;if(!state)return 0;
 static struct dkrr_manifest old,current;if(dkrr_manifest_read(journal,&old)<0)return -1;
 char history[DKRR_PATH],name[128];snprintf(name,sizeof(name),"%s-r%u",old.build,old.sequence);if(join(history,install,"history")<0||join(backup,history,name)<0)return -1;
 int present=exists_dir(live),saved=exists_dir(backup);if(present<0||saved<0)return -1;
 if(saved==1&&dkrr_manifest_check(backup,&old,1)<0)return -1;
 if(present==0&&saved==1){if(dkrr_move_app_directory(backup,live)<0||dkrr_sync_parent(live)<0||dkrr_sync_parent(backup)<0)return -1;status("Interrupted update: previous version restored.");}
 else if(present==1&&saved==0){if(dkrr_manifest_check(live,&old,1)<0)return -1;}
 else if(present==1&&saved==1){if(own_manifest(live,&current)<0)return -1;}
 else return -1;
 if(unlink(journal)<0||dkrr_sync_parent(journal)<0)return -1;return 0;
}
static int ensure_parent(const char *root,const char *relative) {
 char path[DKRR_PATH];if(join(path,root,relative)<0)return -1;
 size_t start=strlen(root)+1,n=strlen(path);
 for(size_t i=start;i<n;++i)if(path[i]=='/'){path[i]=0;int r=dkrr_mkdir(path,0755);path[i]='/';if(r<0)return -1;}
 return 0;
}
/* A native title runs as uid 1. Grant directory-entry replacement only in
 * this verified app tree; never chmod /data, /homebrew or another title.
 * File contents, saved games and system metadata are not permission targets.
 * The in-game updater can then perform its own atomic file replacements. */
static int native_update_access(const char *app) {
#ifdef DKRR_HOST_TEST
 const uid_t app_uid=getuid();
#else
 const uid_t app_uid=1;
#endif
 for(size_t i=0;i<=sizeof(release_files)/sizeof(release_files[0]);++i){
  char path[DKRR_PATH];
  if(i==sizeof(release_files)/sizeof(release_files[0])){if(strlen(app)>=sizeof(path))return -1;strcpy(path,app);}
  else if(join(path,app,release_files[i].path)<0)return -1;
  size_t start=strlen(app);
  for(size_t at=start;at<=strlen(path);++at)if(at==start||(path[at]=='/'&&at>start)){
   char saved=path[at];path[at]=0;
   int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);struct stat st;
   int good=fd>=0&&fstat(fd,&st)==0&&S_ISDIR(st.st_mode)&&fchmod(fd,0755)==0&&fchown(fd,app_uid,(gid_t)-1)==0&&fsync(fd)==0;
   if(fd>=0&&close(fd)<0)good=0;path[at]=saved;if(!good)return -1;
  }
 }
 return 0;
}
static int space_available(int app_required) {
 unsigned long long required=app_required?24ULL*1024*1024:0;
 if(app_required)for(size_t i=0;i<sizeof(release_files)/sizeof(release_files[0]);++i)required+=(unsigned long long)(release_files[i].end-release_files[i].start);
 for(unsigned i=0;i<2;++i){char p[DKRR_PATH];snprintf(p,sizeof(p),"%s/dkr.us.v%u.z64",owned,i?80:77);if(dkrr_regular(p,1)==0)required+=DKRR_ROM_SIZE+8*1024*1024;}
 if(!required)return 1;
 uint64_t available=0;const int checked=dkrr_space_check(DKRR_INSTALL_ROOT,required,&available);
 if(checked){printf("DKRR INSTALL: estimated free space needed=%llu MiB; available=%llu MiB\n",
  (required+1024*1024-1)/(1024*1024),(unsigned long long)(available/(1024*1024)));}
 else puts("DKRR INSTALL: available-space telemetry unavailable; write failures remain guarded.");
 return checked>=0;
}
static int prepare_stage(struct dkrr_manifest *m) {
 int present=exists_dir(stage);if(present<0)return -1;
 if(!present&&dkrr_mkdir(stage,0755)<0)return -1;
 char marker[DKRR_PATH];if(join(marker,stage,".dkrr-stage-id")<0)return -1;
 // A matching marker and exact content-prefix checks allow safe interrupted
 // staging resume; a different/unknown staging directory remains untouched.
 if(present){
  char expected[41],actual[41];unsigned char digest[20];unsigned long long n;
  dkrr_sha1_bytes(release_stage_id,strlen(release_stage_id),digest);dkrr_hex(digest,expected);
  if(dkrr_hash_file(marker,actual,&n)<0||n!=strlen(release_stage_id)||strcmp(expected,actual)){
   // Only an empty stage or its sole interrupted marker can be initialized.
   DIR *d=opendir(stage);if(!d)return -1;struct dirent *e;int empty=1;
   while((e=readdir(d)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,"..")&&strcmp(e->d_name,".dkrr-stage-id")){empty=0;break;}
   closedir(d);if(!empty)return -1;
  }
 }
 if(dkrr_write_new(marker,release_stage_id,strlen(release_stage_id),0644)<0)return -1;
 memset(m,0,sizeof(*m));strcpy(m->build,release_build_id);m->sequence=release_sequence;
 for(size_t i=0;i<sizeof(release_files)/sizeof(release_files[0]);++i){const struct release_file *f=&release_files[i];char p[DKRR_PATH];
  if(ensure_parent(stage,f->path)<0||join(p,stage,f->path)<0||dkrr_write_new(p,f->start,(size_t)(f->end-f->start),f->mode)<0||dkrr_manifest_add(m,stage,f->path,f->mode)<0)return -1;
  const struct dkrr_file *actual=&m->files[m->count-1];if(strcmp(actual->sha1,f->sha1))return -1;
 }
 char audio[DKRR_PATH],folder[DKRR_PATH];if(ensure_parent(stage,"assets/controller-audio/placeholder")<0||join(audio,stage,"assets/controller-audio")<0)return -1;
 for(unsigned i=0;i<2;++i){unsigned revision=i?80:77;char name[80],rompath[DKRR_PATH];snprintf(name,sizeof(name),"dkr.us.v%u.z64",revision);if(join(rompath,owned,name)<0)return -1;
  unsigned char *rom;unsigned rev;if(dkrr_rom_identify(rompath,&rom,&rev)<0||rev!=revision)return -1;
  snprintf(name,sizeof(name),"us-v%u",revision);if(join(folder,audio,name)<0){free(rom);return -1;}
  status(revision==77?"Preparing controller sounds for US 1.0...":"Preparing controller sounds for US 1.1...");
  int rc=dkrr_audio_prepare(rom,DKRR_ROM_SIZE,revision,folder);free(rom);if(rc<0)return -1;
  for(unsigned j=0;j<DKRR_CONTROLLER_CUE_COUNT;++j){snprintf(name,sizeof(name),"assets/controller-audio/us-v%u/%u.pcm",revision,j);if(dkrr_manifest_add(m,stage,name,0644)<0)return -1;}
 }
 char manifest[DKRR_PATH];if(join(manifest,stage,"dkrr-install.manifest")<0)return -1;
 int found=dkrr_regular(manifest,1);
 if(found<0)return -1;
 if(found==0&&dkrr_manifest_write(manifest,m)<0)return -1;
 static struct dkrr_manifest check;return own_manifest(stage,&check)==0&&same_manifest(m,&check)?0:-1;
}
int main(void) {
 setvbuf(stdout,NULL,_IOLBF,0);setvbuf(stderr,NULL,_IOLBF,0);
#if DKRR_NETWORK_UPDATE
 if(dkrr_network_commit()<0)return 10;
#endif
 if(join(data,DKRR_INSTALL_ROOT,"dkrr")<0||join(owned,data,"roms")<0||join(install,data,"installation")<0||join(live,DKRR_INSTALL_ROOT,"homebrew/PPSA94678")<0)return 1;
 char staged_name[160];snprintf(staged_name,sizeof(staged_name),"staged-%s",release_stage_id);
 if(join(stage,install,staged_name)<0||join(journal,install,"transaction.manifest")<0)return 1;
 if(!title_identity_available()){status("Title ID is already registered with different metadata; no app files changed.");return 1;}
 int had_live=exists_dir(live);if(had_live<0)return 1;
 // Data directories preserve existing permissions; no profile file is a target.
 if(dkrr_mkdir(data,0755)<0)return 2;
 const char *leaves[]={"roms","renderer","preferences","profile-us-v77","profile-us-v80","diagnostics","installation"};
 for(size_t i=0;i<sizeof(leaves)/sizeof(leaves[0]);++i){char p[DKRR_PATH];unsigned mode=i==5?01777:i==6?0700:(i>=1&&i<=4)?0777:0755;if(join(p,data,leaves[i])<0||dkrr_mkdir(p,mode)<0)return 2;}
 char lockpath[DKRR_PATH];if(join(lockpath,data,"profile-us-v77/.app-run.lock")<0)return 2;
 int lock=open(lockpath,O_RDWR|O_CREAT|O_NOFOLLOW,0666);struct stat st;
 int leased=-1;
 if(lock>=0&&fstat(lock,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink<=1) {
#if DKRR_NETWORK_UPDATE
  // The in-game client has committed and requested orderly shutdown. Its
  // shared lease remains held until every game I/O worker has stopped.
  struct timespec start,now;
  if(clock_gettime(CLOCK_MONOTONIC,&start)==0)do {
   leased=flock(lock,LOCK_EX|LOCK_NB);if(leased==0)break;
   if(errno!=EWOULDBLOCK&&errno!=EAGAIN)break;
   usleep(100000);
   if(clock_gettime(CLOCK_MONOTONIC,&now)<0)break;
  }while(now.tv_sec-start.tv_sec<120);
#else
  leased=flock(lock,LOCK_EX|LOCK_NB);
#endif
 }
 if(leased<0){if(lock>=0)close(lock);status("DKR-R is running or another install is active. Close it first.");return 2;}
 if(fchmod(lock,0666)<0){close(lock);return 2;}
 if(recover()<0){status("Interrupted installation needs inspection; existing files preserved.");close(lock);return 3;}
 {
  char pending[DKRR_PATH];
  if(join(pending,data,"preferences/update-client-1/native-install")<0){close(lock);return 3;}
  int found=app_update_pending(pending);
  if(found<0){printf("DKRR INSTALL: update-state inspection failed: errno=%d (%s)\n",errno,strerror(errno));status("The in-game update state could not be checked. Existing files were preserved.");close(lock);return 3;}
  if(found>0){status("An in-game update is pending. Start and close DKR-R once to finish recovery before using this installer.");close(lock);return 3;}
 }
 had_live=exists_dir(live);static struct dkrr_manifest old,new;
 if(!had_live&&DKRR_UPDATE_ONLY){status("No existing DKR-R installation. Use the installer ELF first.");close(lock);return 1;}
 if(had_live==1&&known_at(live,&old)<0){status("Existing app differs from its record; nothing will be replaced.");close(lock);return 3;}
 if(had_live&&(old.sequence>release_sequence||(strcmp(old.build,release_build_id)&&old.sequence==release_sequence))){status("Installed version is newer or has the same release sequence; downgrade refused.");close(lock);return 3;}
 struct dkrr_rom inputs[2];status("Checking both owned US ROMs; USB: DKRR folder or drive root.");
 if(dkrr_rom_find(DKRR_USB_ROOT,owned,inputs)<0){status("Both clean US ROMs are required. Put 1.0 and 1.1 in USB/DKRR and run again.");close(lock);return 4;}
 if(!space_available(!(had_live&&!strcmp(old.build,release_build_id)&&old.sequence==release_sequence))){status("Not enough free internal space to stage the update and retain recovery data.");close(lock);return 4;}
 if(dkrr_rom_install(owned,inputs)<0){status("ROM import failed; existing game is preserved.");close(lock);return 4;}
 if(had_live&&!strcmp(old.build,release_build_id)&&old.sequence==release_sequence){
  int matches=old.count==sizeof(release_files)/sizeof(release_files[0])+2*DKRR_CONTROLLER_CUE_COUNT;
  for(size_t i=0;i<sizeof(release_files)/sizeof(release_files[0])&&matches;++i){char path[DKRR_PATH],hash[41];unsigned long long n;
   const struct release_file *f=&release_files[i];if(join(path,live,f->path)<0||dkrr_hash_file(path,hash,&n)<0||n!=(unsigned long long)(f->end-f->start)||strcmp(hash,f->sha1))matches=0;
  }
  if(!matches){status("Recorded build identity has different package files; refusing an ambiguous update.");close(lock);return 3;}
  if(native_update_access(live)<0){status("App verified; native update permissions could not be prepared.");close(lock);return 9;}
  if(dkrr_register_source(DKRR_INSTALL_ROOT,live)<0){status("App verified; ShadowMountPlus registration pending. Start it and rerun this ELF.");close(lock);return 9;}
  status("This version is already installed and verified.");puts(DKRR_UPDATE_ONLY?"DKRR UNIVERSAL UPDATE PASS.":"DKRR UNIVERSAL INSTALL PASS.");close(lock);return 0;
 }
 status("Preparing app and controller sounds from your ROMs.");
 if(prepare_stage(&new)<0){status("Staging verification failed. Existing game is preserved; staged files kept for diagnosis.");close(lock);return 5;}
 if(native_update_access(stage)<0){status("Native update access could not be prepared; existing game is preserved.");close(lock);return 5;}
 // Require identical native metadata on upgrades: no shell-cache migration.
 if(had_live){char a[DKRR_PATH],b[DKRR_PATH],x[41],y[41];unsigned long long nx,ny;
  if(join(a,live,"sce_sys/param.json")<0||join(b,stage,"sce_sys/param.json")<0||dkrr_hash_file(a,x,&nx)<0||dkrr_hash_file(b,y,&ny)<0||nx!=ny||strcmp(x,y)){status("Title metadata change requires a qualified migration; old app preserved.");close(lock);return 6;}
  char history[DKRR_PATH],old_name[128];snprintf(old_name,sizeof(old_name),"%s-r%u",old.build,old.sequence);if(join(history,install,"history")<0||dkrr_mkdir(history,0700)<0||join(backup,history,old_name)<0){close(lock);return 6;}
  if(dkrr_manifest_check(live,&old,1)<0||exists_dir(backup)!=0){status("App changed or a previous recovery folder is still present; refusing replacement.");close(lock);return 6;}
  // Cross-directory moves need both directory entries durable. Keep the
  // journal if either sync fails, including after a successful rename.
  if(dkrr_manifest_write(journal,&old)<0||dkrr_move_app_directory(live,backup)<0||dkrr_sync_parent(backup)<0||dkrr_sync_parent(live)<0){close(lock);return 7;}
 }
 if(!had_live){char parent[DKRR_PATH];if(join(parent,DKRR_INSTALL_ROOT,"homebrew")<0||dkrr_mkdir(parent,0755)<0){close(lock);return 7;}}
 if(dkrr_move_app_directory(stage,live)<0){
  if(had_live&&dkrr_move_app_directory(backup,live)==0){dkrr_sync_parent(live);dkrr_sync_parent(backup);}
  status("Activation failed. Rerun this ELF for recovery.");close(lock);return 7;
 }
 if(dkrr_sync_parent(live)<0||dkrr_sync_parent(stage)<0||own_manifest(live,&new)<0){close(lock);return 8;}
 if(had_live&&(unlink(journal)<0||dkrr_sync_parent(journal)<0)){close(lock);return 8;}
 if(dkrr_register_source(DKRR_INSTALL_ROOT,live)<0){status("App installed; ShadowMountPlus registration pending. Start it and rerun this ELF.");close(lock);return 9;}
 status("Files installed and verified. ShadowMountPlus will discover the PS5 icon.");
 puts(DKRR_UPDATE_ONLY?"DKRR UNIVERSAL UPDATE PASS.":"DKRR UNIVERSAL INSTALL PASS.");close(lock);return 0;
}
