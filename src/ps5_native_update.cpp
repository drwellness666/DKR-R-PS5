// SPDX-License-Identifier: GPL-3.0-or-later
// Data-only, in-process update transaction. No ELF execution, socket, daemon,
// kernel access or writes outside this installation and its private journal.
#include "ps5_native_update.hpp"
#include "ps5_update_files.h"
#include "ps5_storage_budget.h"
#include "../installer/dkrr_install.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <set>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#ifndef DKRR_UPDATE_ROOT
#define DKRR_UPDATE_ROOT "/data/dkrr"
#endif
#ifndef DKRR_NATIVE_APP
#define DKRR_NATIVE_APP "/data/homebrew/PPSA94678"
#endif
#ifndef DKRR_RELEASE_SEQUENCE
#error "Compile with the installed release sequence"
#endif
#ifdef DKRR_NATIVE_TEST_HOOK
extern "C" void dkrr_native_test_hook(const char*,unsigned);
#define CHECKPOINT(name,index) dkrr_native_test_hook(name,index)
#else
#define CHECKPOINT(name,index) ((void)0)
#endif
namespace dkrr::updates::native {
namespace {
using Path=std::string;
constexpr std::size_t public_count=sizeof(dkrr_public_files)/sizeof(*dkrr_public_files);
struct Context {
    Path client=Path(DKRR_UPDATE_ROOT)+"/preferences/update-client-"+std::to_string(unsigned(getuid()));
    Path root=client+"/native-install";
    Path app=DKRR_NATIVE_APP;
    char* error;std::size_t capacity;
    bool fail(const char* message) {std::snprintf(error,capacity,"%s",message);return false;}
};
bool read_all(int fd,void* data,std::size_t size) {
    auto* out=static_cast<unsigned char*>(data);
    while(size){auto n=read(fd,out,size);if(n<0&&errno==EINTR)continue;if(n<=0||std::size_t(n)>size)return false;out+=n;size-=std::size_t(n);}return true;
}
bool write_all(int fd,const void* data,std::size_t size) {
    auto* in=static_cast<const unsigned char*>(data);
    while(size){auto n=write(fd,in,size);if(n<0&&errno==EINTR)continue;if(n<=0||std::size_t(n)>size)return false;in+=n;size-=std::size_t(n);}return true;
}
bool ordinary_directory(const Path& path) {
    if(path.empty()||path[0]!='/'||path.size()>=DKRR_PATH)return false;
    struct stat st{};
    for(std::size_t i=1;i<=path.size();++i)if(i==path.size()||path[i]=='/')
        if(lstat(path.substr(0,i).c_str(),&st)<0||!S_ISDIR(st.st_mode))return false;
    return true;
}
bool private_directory(const Path& path,bool create=false) {
    if(create) {
        const int created=mkdir(path.c_str(),0700);
        if(created<0&&errno!=EEXIST)return false;
        if(created==0&&dkrr_sync_parent(path.c_str())<0)return false;
    }
    struct stat st{};return ordinary_directory(path)&&lstat(path.c_str(),&st)==0&&st.st_uid==getuid()&&(st.st_mode&077)==0;
}
bool parents(const Path& root,const Path& name) {
    for(std::size_t i=0;i<name.size();++i)if(name[i]=='/'&&!private_directory(root+"/"+name.substr(0,i),true))return false;
    return true;
}
bool sync(const Path& file){return dkrr_sync_parent(file.c_str())==0;}
bool touch(const Path& path,const char* text) {
    return dkrr_write_new(path.c_str(),text,std::strlen(text),0600)==0;
}
bool marker(const Path& path,const char* text) {
    int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return false;
    struct stat st{};char data[16];const auto size=std::strlen(text);
    bool ok=size<sizeof(data)&&fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink<=1&&st.st_uid==getuid()&&st.st_size==static_cast<off_t>(size)&&read_all(fd,data,size)&&!std::memcmp(data,text,size);
    close(fd);return ok;
}
bool present(const Path& path){
    struct stat st{};if(lstat(path.c_str(),&st)==0)return true;
    if(errno==ENOENT)return false;
    // A metadata/I/O error is not evidence of absence. In particular it must
    // never turn an active transaction into an abandoned staging directory.
    throw std::runtime_error("Update entry metadata is unavailable");
}
bool copy_stream(int source,const Path& target,std::uint64_t size,unsigned mode) {
    int fd=open(target.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,mode);if(fd<0)return false;
    unsigned char buffer[65536];bool ok=true;
    while(ok&&size){auto n=std::size_t(std::min<std::uint64_t>(size,sizeof(buffer)));ok=read_all(source,buffer,n)&&write_all(fd,buffer,n);size-=n;}
    if(fchmod(fd,mode)<0||fsync(fd)<0)ok=false;if(close(fd)<0)ok=false;return ok&&sync(target);
}
bool copy_file(const Path& source,const Path& target,unsigned mode) {
    if(dkrr_regular(source.c_str(),0)!=1)return false;
    int fd=open(source.c_str(),O_RDONLY|O_NOFOLLOW);if(fd<0)return false;struct stat st{};
    bool ok=fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink<=1&&st.st_size>=0&&std::uint64_t(st.st_size)<=payload_limit&&copy_stream(fd,target,std::uint64_t(st.st_size),mode);
    close(fd);return ok;
}
const dkrr_file* find(const dkrr_manifest& manifest,const char* path) {
    for(std::size_t i=0;i<manifest.count;++i)if(!std::strcmp(manifest.files[i].path,path))return &manifest.files[i];return nullptr;
}
bool matches(const Path& path,const dkrr_file& entry) {
    char digest[41];unsigned long long bytes;
    return dkrr_hash_file(path.c_str(),digest,&bytes)==0&&bytes==entry.size&&!std::strcmp(digest,entry.sha1);
}
bool valid_inventory(const dkrr_manifest& m) {
    if(m.count!=public_count+64)return false;
    for(auto* name:dkrr_public_files)if(!find(m,name))return false;
    for(unsigned revision:{77U,80U})for(unsigned cue=0;cue<32;++cue){char name[128];std::snprintf(name,sizeof(name),"assets/controller-audio/us-v%u/%u.pcm",revision,cue);if(!find(m,name))return false;}
    return true;
}
bool same_inventory(const dkrr_manifest& a,const dkrr_manifest& b) {
    if(a.sequence!=b.sequence||std::strcmp(a.build,b.build)||a.count!=b.count)return false;
    for(std::size_t i=0;i<a.count;++i){const auto& x=a.files[i];const auto* y=find(b,x.path);if(!y||x.size!=y->size||x.mode!=y->mode||std::strcmp(x.sha1,y->sha1))return false;}return true;
}
bool writable(const Path& directory) {
    // No probe entry is left in the mounted app if power is lost during
    // preflight. The installer owns the one-time directory grant.
    struct stat st{};
    return ordinary_directory(directory)&&lstat(directory.c_str(),&st)==0&&st.st_uid==getuid()&&(st.st_mode&0300)==0300&&access(directory.c_str(),W_OK|X_OK)==0;
}
bool preflight(Context& c,const dkrr_manifest& old) {
    if(!ordinary_directory(c.app)||!valid_inventory(old)||dkrr_manifest_check(c.app.c_str(),&old,1)<0)return c.fail("The installed files differ from their record. Nothing was replaced.");
    std::set<Path> directories{c.app};
    for(auto* name:dkrr_public_files){Path n=name;const auto slash=n.rfind('/');if(slash!=n.npos)directories.insert(c.app+"/"+n.substr(0,slash));}
    for(const auto& d:directories)if(!writable(d))return c.fail("This installation does not grant DKR-R access to its own program folder. Run the new installer once.");
    return true;
}
bool allowed_cleanup(const Path& name,bool directory) {
    if(!directory&&(name=="old.manifest"||name=="old.manifest.tmp"||name=="new.manifest"||name=="new.manifest.tmp"||name=="ready"||name=="applying"||name=="committed"))return true;
    for(auto* branch:{"old/","new/"}){
        Path b=branch;if(directory&&name==b.substr(0,b.size()-1))return true;
        for(std::size_t i=0;i<=public_count;++i){Path p=b+(i==public_count?"dkrr-install.manifest":dkrr_public_files[i]);
            if(!directory&&(name==p||name==p+".tmp"))return true;
            if(directory&&p.starts_with(name+"/"))return true;
        }
    }return false;
}
bool collect_cleanup(const Path& root,const Path& relative,std::vector<Path>& files,std::vector<Path>& directories) {
    if(relative.size()>200||directories.size()>64||files.size()>100)return false;
    const auto path=relative.empty()?root:root+"/"+relative;
    if(!private_directory(path))return false;
    DIR* dir=opendir(path.c_str());if(!dir)return false;bool ok=true;
    for(;;){
        // A null entry is also returned for I/O errors. Reset errno at each
        // call: metadata checks and recursive walks can change it meanwhile.
        errno=0;auto* entry=readdir(dir);
        if(!entry){if(errno)ok=false;break;}
        if(!std::strcmp(entry->d_name,".")||!std::strcmp(entry->d_name,".."))continue;
        Path name=relative.empty()?entry->d_name:relative+"/"+entry->d_name,full=root+"/"+name;struct stat st{};
        if(lstat(full.c_str(),&st)<0||st.st_uid!=getuid()){ok=false;break;}
        if(S_ISDIR(st.st_mode)){if(!allowed_cleanup(name,true)||!collect_cleanup(root,name,files,directories)){ok=false;break;}directories.push_back(full);}
        else if(S_ISREG(st.st_mode)&&st.st_nlink<=1&&allowed_cleanup(name,false))files.push_back(full);
        else {ok=false;break;}
    }if(closedir(dir)<0)ok=false;return ok;
}
bool cleanup(Context& c) {
    std::vector<Path> files,dirs;if(!collect_cleanup(c.root,"",files,dirs))return c.fail("Update recovery data could not be fully checked. It was kept for the next recovery attempt.");
    // Callers with an active transaction have verified a complete old or new
    // installation. Retire that transaction durably before destroying either
    // journal or backup. Directory enumeration order is unspecified; a crash
    // during later deletion must only resume cleanup, never need missing data.
    const auto active=c.root+"/applying";
    auto entry=std::find(files.begin(),files.end(),active);
    if(entry!=files.end()) {
        if(unlink(active.c_str())<0||!sync(active))return false;
        files.erase(entry);CHECKPOINT("retired",0);
    }
    for(const auto& p:files)if(unlink(p.c_str())<0)return false;
    // collect_cleanup appends children before their parents.
    for(const auto& p:dirs)if(rmdir(p.c_str())<0)return false;
    return rmdir(c.root.c_str())==0&&sync(c.root);
}
bool load_journal(Context& c,dkrr_manifest& old,dkrr_manifest& next) {
    return private_directory(c.root)&&dkrr_manifest_read((c.root+"/old.manifest").c_str(),&old)==0&&valid_inventory(old)&&
        dkrr_manifest_read((c.root+"/new.manifest").c_str(),&next)==0&&valid_inventory(next)&&next.sequence>old.sequence;
}
bool backup_matches(Context& c,const dkrr_manifest& old) {
    for(auto* name:dkrr_public_files){const auto* row=find(old,name);if(!row||!matches(c.root+"/old/"+name,*row))return false;}
    dkrr_manifest backup{};return dkrr_manifest_read((c.root+"/old/dkrr-install.manifest").c_str(),&backup)==0&&same_inventory(old,backup);
}
bool rollback(Context& c,const dkrr_manifest& old) {
    if(!backup_matches(c,old))return c.fail("The update recovery copy could not be verified. Existing files were preserved.");
    for(std::size_t i=0;i<=public_count;++i){const char* name=i==public_count?"dkrr-install.manifest":dkrr_public_files[i];
        Path target=c.app+"/"+name,temp=target+".dkrr-restore";
        // A previous interrupted restoration only leaves this exact private
        // replacement entry; unlinking it never follows or mutates its inode.
        if(present(temp)){struct stat st{};if(lstat(temp.c_str(),&st)<0||!S_ISREG(st.st_mode)||st.st_uid!=getuid()||st.st_nlink>1||unlink(temp.c_str())<0)return false;}
        // The first activation can be denied by the console's filesystem
        // policy even after staging succeeded. An unchanged file needs no
        // restoration write. This also makes restart recovery possible while
        // that denial persists, instead of blocking the old game at startup.
        if(i<public_count&&matches(target,*find(old,name)))continue;
        if(i==public_count){dkrr_manifest installed{};if(dkrr_manifest_read(target.c_str(),&installed)==0&&same_inventory(old,installed))continue;}
        unsigned mode=i==public_count?0644:find(old,name)->mode;
        if(!copy_file(c.root+"/old/"+name,temp,mode)||rename(temp.c_str(),target.c_str())<0||!sync(target))return c.fail("Recovery was interrupted. Start DKR-R again to complete it.");
        CHECKPOINT("restored",unsigned(i));
    }
    if(dkrr_manifest_check(c.app.c_str(),&old,1)<0)return c.fail("Restored files could not be verified. Start DKR-R again.");
    return cleanup(c);
}
std::uint64_t little(const unsigned char* p,unsigned n){std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(p[i])<<(8*i);return v;}
bool canonical_string(const unsigned char* data,std::size_t size,const char* expected) {
    auto n=std::strlen(expected);return n<size&&!std::memcmp(data,expected,n)&&std::all_of(data+n,data+size,[](unsigned char c){return c==0;});
}
bool prepare_impl(Context& c,int fd,const Offer& offer) {
    if(!offer.native_package||offer.sequence<=DKRR_RELEASE_SEQUENCE||!payload_digest(fd,offer))return c.fail("The native update signature or package digest is invalid.");
    if(!private_directory(c.client)||!sync(c.client))return c.fail("The private update folder is unavailable or could not be synchronized.");
    if(present(c.root))return c.fail("An earlier update needs recovery. Close and restart DKR-R first.");
    auto manifests=std::make_unique<std::array<dkrr_manifest,2>>();
    auto& old=(*manifests)[0];auto& next=(*manifests)[1];
    if(dkrr_manifest_read((c.app+"/dkrr-install.manifest").c_str(),&old)<0||old.sequence!=DKRR_RELEASE_SEQUENCE||!preflight(c,old))return c.fail("The installation is not ready for native updates. Run the current installer once.");
    unsigned char header[96];
    if(!read_all(fd,header,sizeof(header))||std::memcmp(header,"DKRRPK2\n",8)||little(header+8,4)!=offer.sequence||little(header+12,4)!=public_count)return c.fail("The native package header is invalid.");
    char build[80];std::memcpy(build,header+16,80);build[79]=0;
    if(std::strlen(build)!=35||std::strncmp(build,"integrated-",11)||!canonical_string(header+16,80,build))return c.fail("The native package build identity is invalid.");
    for(unsigned i=11;i<35;++i)if(!((build[i]>='0'&&build[i]<='9')||(build[i]>='a'&&build[i]<='f')))return c.fail("The native package build identity is invalid.");
    next=old;std::snprintf(next.build,sizeof(next.build),"%s",build);next.sequence=offer.sequence;
    if(!private_directory(c.root,true)||!private_directory(c.root+"/new",true)||!private_directory(c.root+"/old",true))return c.fail("The update staging folder could not be created.");
    for(auto* name:dkrr_public_files){
        unsigned char row[192];if(!read_all(fd,row,sizeof(row))||!canonical_string(row,160,name))return c.fail("Unexpected or duplicate file in the native package.");
        auto bytes=little(row+160,8),mode=little(row+168,4);
        const unsigned expected_mode=(!std::strcmp(name,"eboot.bin")||!std::strcmp(name,"sce_module/libc.prx"))?0755:0644;
        if(bytes>payload_limit||mode!=expected_mode)return c.fail("Invalid native file size or permissions.");
        auto* entry=const_cast<dkrr_file*>(find(next,name));const auto* previous=find(old,name);if(!entry||!previous)return false;
        entry->size=bytes;entry->mode=unsigned(mode);dkrr_hex(row+172,entry->sha1);
        if((!std::strcmp(name,"sce_sys/param.json")||!std::strcmp(name,"sce_module/libc.prx"))&&(previous->size!=bytes||previous->mode!=mode||std::strcmp(previous->sha1,entry->sha1)))return c.fail("This update changes registered title metadata or the runtime ABI and needs a qualified migration. The installed game is unchanged.");
        if(!parents(c.root+"/new",name)||!copy_stream(fd,c.root+"/new/"+name,bytes,unsigned(mode))||!matches(c.root+"/new/"+name,*entry))return c.fail("The staged update file failed verification.");
        if(!std::strcmp(name,"eboot.bin")) {
            int executable=open((c.root+"/new/eboot.bin").c_str(),O_RDONLY|O_NOFOLLOW);unsigned char magic[4];
            bool valid=executable>=0&&read_all(executable,magic,4)&&magic[0]==0x4f&&magic[1]==0x15&&magic[2]==0x3d&&magic[3]==0x1d;
            if(executable>=0)close(executable);if(!valid)return c.fail("The native executable container is invalid.");
        }
    }
    if(lseek(fd,0,SEEK_CUR)!=static_cast<off_t>(offer.bytes))return c.fail("The native package contains trailing or missing data.");
    if(dkrr_manifest_write((c.root+"/old.manifest").c_str(),&old)<0||dkrr_manifest_write((c.root+"/new.manifest").c_str(),&next)<0||dkrr_manifest_write((c.root+"/new/dkrr-install.manifest").c_str(),&next)<0)return c.fail("The update recovery journal could not be saved.");
    for(auto* name:dkrr_public_files)if(!parents(c.root+"/old",name)||!copy_file(c.app+"/"+name,c.root+"/old/"+name,find(old,name)->mode))return c.fail("The recovery copy could not be prepared. The installed game is unchanged.");
    if(!copy_file(c.app+"/dkrr-install.manifest",c.root+"/old/dkrr-install.manifest",0644)||!backup_matches(c,old)||!touch(c.root+"/ready","READY\n"))return c.fail("The recovery copy could not be verified. The installed game is unchanged.");
    CHECKPOINT("prepared",0);return true;
}
int recover_impl(Context& c,int lease) {
    if(!present(c.root))return 0;
    if(lease<0||flock(lease,LOCK_EX|LOCK_NB)<0){c.fail("Update recovery is waiting for another DKR-R process.");return -1;}
    int result=-1;
    if(!private_directory(c.root)){c.fail("The update recovery directory is not private.");}
    else if(!present(c.root+"/applying")){result=cleanup(c)?0:-1;}
    else {
        auto manifests=std::make_unique<std::array<dkrr_manifest,2>>();
        auto& old=(*manifests)[0];auto& next=(*manifests)[1];
        if(!marker(c.root+"/ready","READY\n")||!load_journal(c,old,next))c.fail("The update recovery journal could not be verified.");
        else if(!marker(c.root+"/applying","APPLY\n")) {
            // An incomplete commit marker cannot precede file activation.
            // Require the complete old installation before abandoning it.
            if(dkrr_manifest_check(c.app.c_str(),&old,1)==0)result=cleanup(c)?0:-1;
            else c.fail("The activation marker is incomplete and the old installation does not match.");
        }
        else if(marker(c.root+"/committed","DONE\n")&&dkrr_manifest_check(c.app.c_str(),&next,1)==0){result=cleanup(c)?0:-1;}
        else if(rollback(c,old)){c.fail("An interrupted update was restored. Start DKR-R again.");result=1;}
    }
    if(flock(lease,LOCK_SH|LOCK_NB)<0)return -1;return result;
}
bool apply_impl(Context& c,int lease) {
    if(lease<0||flock(lease,LOCK_EX|LOCK_NB)<0)return c.fail("Another DKR-R process is still using the installation.");
    auto manifests=std::make_unique<std::array<dkrr_manifest,2>>();
    auto& old=(*manifests)[0];auto& next=(*manifests)[1];
    if(!marker(c.root+"/ready","READY\n")||!load_journal(c,old,next)||old.sequence!=DKRR_RELEASE_SEQUENCE||dkrr_manifest_check(c.app.c_str(),&old,1)<0||!backup_matches(c,old))return c.fail("The update no longer matches the installed game. No program file was changed.");
    for(auto* name:dkrr_public_files)if(!matches(c.root+"/new/"+name,*find(next,name)))return c.fail("The prepared update file changed. No program file was replaced.");
    dkrr_manifest recorded{};if(dkrr_manifest_read((c.root+"/new/dkrr-install.manifest").c_str(),&recorded)<0||!same_inventory(next,recorded))return c.fail("The prepared installation record changed.");
    if(!touch(c.root+"/applying","APPLY\n"))return c.fail("The update commit journal could not be saved.");
    bool ok=true;
    // Every replacement is atomic; the directory itself is never renamed.
    // Keep eboot last among program files, followed only by the manifest.
    std::vector<const char*> names;for(auto* n:dkrr_public_files)if(std::strcmp(n,"eboot.bin"))names.push_back(n);names.push_back("eboot.bin");names.push_back("dkrr-install.manifest");
    for(std::size_t i=0;i<names.size();++i){auto from=c.root+"/new/"+names[i],to=c.app+"/"+names[i];
        const auto* previous=find(old,names[i]);const auto* updated=find(next,names[i]);
        const bool identical=previous&&updated&&previous->size==updated->size&&previous->mode==updated->mode&&!std::strcmp(previous->sha1,updated->sha1);
        // In particular, never replace the already loaded, identical libc or
        // the unchanged shell metadata. Only changed program entries move.
        if(!identical&&(rename(from.c_str(),to.c_str())<0||!sync(to)||!sync(from))){ok=false;break;}
        CHECKPOINT("replaced",unsigned(i));
    }
    if(ok)ok=dkrr_manifest_check(c.app.c_str(),&next,1)==0&&touch(c.root+"/committed","DONE\n");
    if(!ok){const bool restored=rollback(c,old);return c.fail(restored?"Update stopped; the previous version was restored.":"Update stopped. Start DKR-R again to complete recovery.");}
    CHECKPOINT("committed",0);return true;
}
}
bool storage_budget(std::uint64_t package_bytes,Space& space,char* error,std::size_t capacity) {
    space={};
    try {
        Context c{.error=error,.capacity=capacity};
        if(!package_bytes||package_bytes>payload_limit)return c.fail("Invalid update storage estimate.");
        auto old=std::make_unique<dkrr_manifest>();
        if(!ordinary_directory(c.app)||dkrr_manifest_read((c.app+"/dkrr-install.manifest").c_str(),old.get())<0||
           old->sequence!=DKRR_RELEASE_SEQUENCE||!valid_inventory(*old))
            return c.fail("The installation record could not be read. Run the current installer once.");
        std::uint64_t previous=0,largest=0;
        for(auto* name:dkrr_public_files) {
            const auto* entry=find(*old,name);
            if(!entry||entry->size>payload_limit)return c.fail("Invalid installed-file size in update record.");
            previous+=entry->size;largest=std::max(largest,std::uint64_t(entry->size));
        }
        space.required=2*package_bytes+previous+largest+16ULL*1024*1024;
        const int result=dkrr_space_check(DKRR_UPDATE_ROOT,space.required,&space.available);
        space.known=result!=0;
        if(result<0) {
            constexpr std::uint64_t mib=1024*1024;
            std::snprintf(error,capacity,"Update needs about %llu MiB free; %llu MiB available. Nothing was downloaded.",
                (unsigned long long)((space.required+mib-1)/mib),(unsigned long long)(space.available/mib));
            return false;
        }
        return true;
    } catch(...) {std::snprintf(error,capacity,"Update storage could not be checked. The installed game is unchanged.");return false;}
}
bool prepare(int fd,const Offer& offer,char* error,std::size_t capacity) {
    try {Context c{.error=error,.capacity=capacity};const bool ok=prepare_impl(c,fd,offer);if(!ok&&present(c.root)&&!present(c.root+"/applying"))cleanup(c);return ok;}
    catch(...){std::snprintf(error,capacity,"Native update preparation stopped. The installed game is unchanged.");return false;}
}
int recover(int lease,char* error,std::size_t capacity) {
    try {Context c{.error=error,.capacity=capacity};return recover_impl(c,lease);}
    catch(...){std::snprintf(error,capacity,"Native update recovery stopped. Existing files were preserved.");return -1;}
}
bool apply(int lease,char* error,std::size_t capacity) {
    try {Context c{.error=error,.capacity=capacity};return apply_impl(c,lease);}
    catch(...){std::snprintf(error,capacity,"Native update stopped. Start DKR-R again to complete recovery.");return false;}
}
}
