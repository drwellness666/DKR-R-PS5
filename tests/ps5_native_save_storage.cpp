// SPDX-License-Identifier: GPL-3.0-or-later
// Host filesystem transactions with explicitly modeled missing native metadata.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include "ps5_native_save_policy.hpp"
namespace fs=std::filesystem;
static fs::path fixture_root;
static bool hide_links=false,metadata_error=false,fail_sync=false;
static std::uintmax_t fixture_link_count(const fs::path&p,std::error_code&ec) {
    if(metadata_error){ec=std::make_error_code(std::errc::io_error);return UINTMAX_MAX;}
    if(hide_links){ec.clear();return 0;}
    return fs::hard_link_count(p,ec);
}
static bool fixture_transaction_path(const fs::path&p) {
    const auto relative=p.lexically_relative(fixture_root);
    if(relative.empty() || relative.native().starts_with(".."))return false;
    return dkrr::ps5_saves::transaction_path(fs::path("/data/dkrr")/relative);
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    if(fail_sync){errno=EIO;return -1;}return __real_fsync(fd);
}
#include "save_storage.hpp"
namespace store=dkr::runtime::saves::storage;
static store::Bytes image(unsigned char fill) {
    store::Bytes b(512,fill);b[0]='D';b[1]='K';b[2]='R';return b;
}
static bool valid(const fs::path&p,store::Bytes&b) {
    return store::read_exact(p,512,b)&&b[0]=='D'&&b[1]=='K'&&b[2]=='R';
}
static void expect(const fs::path&p,const store::Bytes&want) {
    store::Bytes got;assert(store::read_exact(p,want.size(),got)&&got==want);
}
int main(int argc,char**argv) {
    std::string error;
    if(argc==4 && std::string(argv[1])=="lease-probe") {
        fixture_root=argv[2];hide_links=true;
        return store::acquire(argv[3],error)?2:0;
    }
    assert(argc==2);fixture_root=argv[1];fs::create_directories(fixture_root);
    assert(dkrr::ps5_saves::transaction_path("/data/dkrr/profile-us-v77/saves/dkr.us.v77.bin"));
    assert(dkrr::ps5_saves::transaction_path("/data/dkrr/profile-us-v80/controller-pak-1.bin.lock"));
    for(const char*p:{"/data/dkrr/preferences/settings.ini","/data/foreign/save.bin",
        "/data/dkrr/profile-us-v77/saves/../settings.ini"})
        assert(!dkrr::ps5_saves::transaction_path(p));
    const auto profile=fixture_root/"profile-us-v77",path=profile/"saves/dkr.us.v77.bin";
    const auto alias=fixture_root/"alias.bin";
    const auto a=image('a'),b=image('b'),c=image('c');
    assert(store::write_atomic(path,a,valid,error));fs::create_hard_link(path,alias);
    assert(!store::write_atomic(path,b,valid,error));expect(path,a);expect(alias,a);
    hide_links=true;
    assert(!store::writable_path(path,error)); // Inspect must not invent link metadata.
    assert(store::write_atomic(path,b,valid,error));
    expect(path,b);expect(alias,a);expect(path.string()+".bak",a);
    assert(!fs::equivalent(path,alias));assert(!fs::is_empty(path.parent_path()/"save-history"/path.filename()));
    const auto foreign=fixture_root/"unrelated.bin";
    {std::ofstream f(foreign);f<<"keep";}
    assert(!store::durable_replace(foreign,c,valid,error));assert(fs::file_size(foreign)==4);
    metadata_error=true;assert(!store::write_atomic(path,c,valid,error));metadata_error=false;expect(path,b);
    fail_sync=true;assert(!store::write_atomic(path,c,valid,error));fail_sync=false;expect(path,b);
    for(const auto&e:fs::directory_iterator(path.parent_path()))assert(e.path().filename().string().find(".pending-")==std::string::npos);
    auto bad=c;bad[0]='?';assert(!store::durable_replace(path,bad,valid,error));expect(path,b);
    const auto broken=profile/"saves/broken.bin";
    {std::ofstream f(broken);f<<"broken";}
    assert(store::write_atomic(broken,c,valid,error));expect(broken,c);
    unsigned preserved=0;
    for(const auto&e:fs::directory_iterator(broken.parent_path()))
        if(e.path().filename().string().starts_with("broken.bin.invalid-")){assert(fs::file_size(e.path())==6);++preserved;}
    assert(preserved==1);
    auto lease=store::acquire(path,error);assert(lease);
    const auto pid=fork();assert(pid>=0);
    if(!pid){execl(argv[0],argv[0],"lease-probe",fixture_root.c_str(),path.c_str(),nullptr);_exit(3);}
    int status=0;assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0);lease.reset();
    for(unsigned char i=0;i<14;++i)assert(store::write_atomic(path,image(i),valid,error));
    unsigned history=0;for(const auto&e:fs::directory_iterator(path.parent_path()/"save-history"/path.filename()))
        if(e.path().extension()==".bin")++history;
    assert(history==8);
    const auto rev_a=fixture_root/"profile-us-v80/saves/dkr.us.v77.bin";
    assert(store::write_atomic(rev_a,a,valid,error));expect(rev_a,a);expect(path,image(13));
    const auto online=profile/"saves/online/fixture/dkr.us.v77.bin";fs::create_directories(online.parent_path());fs::create_hard_link(path,online);
    store::protect_offline(profile,true);assert(!store::write_atomic(path,c,valid,error));
    assert(store::write_atomic(online,c,valid,error));expect(path,image(13));expect(online,c);store::protect_offline(profile,false);
    const auto link=profile/"saves/redirect.bin";fs::create_symlink(path,link);
    if(getuid()!=0){assert(!store::write_atomic(link,c,valid,error));expect(path,image(13));}
    puts("PASS native save transaction backend: scoped unknown metadata, known aliases refused, atomic alias isolation, backup/history, validation and sync failure, invalid preservation, interprocess exclusion, revision separation and online protection");
    puts("Host model only: does not establish native filesystem or power-loss behavior.");
}
