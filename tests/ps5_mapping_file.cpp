// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_mapping_file.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
struct TrackedMutex {
    bool held=false;unsigned locks=0;
    void lock(){assert(!held);held=true;++locks;}
    void unlock(){assert(held);held=false;}
};
static TrackedMutex g_platform_mutex;
static bool observe_import=false,read_unlocked=false;
enum class Fault {None,Interrupt,ReadError,StatError,CloseError,UnknownSize};
static Fault fault=Fault::None;
static int tracked_fd=-1,open_flags=0;
static unsigned reads=0,closes=0;
extern "C" int __real_open(const char*,int,...);
extern "C" ssize_t __real_read(int,void*,size_t);
extern "C" int __real_fstat(int,struct stat*);
extern "C" int __real_close(int);
extern "C" int __wrap_open(const char* path,int flags,...) {
    if(observe_import)read_unlocked=!g_platform_mutex.held;
    open_flags=flags;reads=closes=0;tracked_fd=__real_open(path,flags);return tracked_fd;
}
extern "C" ssize_t __wrap_read(int fd,void* data,size_t size) {
    assert(fd==tracked_fd);++reads;
    if(fault==Fault::Interrupt&&reads==1){errno=EINTR;return -1;}
    if(fault==Fault::ReadError&&reads==2){errno=EIO;return -1;}
    return __real_read(fd,data,size);
}
extern "C" int __wrap_fstat(int fd,struct stat* info) {
    assert(fd==tracked_fd);
    if(fault==Fault::StatError){errno=EIO;return -1;}
    const int result=__real_fstat(fd,info);
    if(!result&&fault==Fault::UnknownSize)info->st_size=0;
    return result;
}
extern "C" int __wrap_close(int fd) {
    assert(fd==tracked_fd);++closes;const int result=__real_close(fd);
    if(fault==Fault::CloseError){errno=EIO;return -1;}return result;
}
#include "mapping_reader.inc"
#include "mapping_key.inc"
namespace dkr::runtime::platform {
enum class InputBackend {SDL2Compat,SDL3Native};
static InputBackend backend=InputBackend::SDL2Compat;
InputBackend active_input_backend(){return backend;}
bool import_controller_mappings(const std::filesystem::path&,std::string&);
}
struct ControllerRecord {int id;};
static std::vector<ControllerRecord> g_controller_devices;
static std::array<void*,4> g_controllers{},g_gyro_controllers{};
static unsigned additions=0,merges=0,closed_devices=0,refreshes=0;
static std::vector<std::string> merged;
static int SDL_GameControllerAddMapping(const char* line) {
    assert(g_platform_mutex.held&&line&&line[0]);++additions;return 1;
}
static bool MergeUserMappings(const std::vector<std::string>& lines,std::string&) {
    assert(g_platform_mutex.held);++merges;merged=lines;return true;
}
static void CloseControllerRecord(ControllerRecord&){assert(g_platform_mutex.held);++closed_devices;}
static void RefreshControllers(){assert(g_platform_mutex.held);++refreshes;}
#include "mapping_import.inc"
static void put(const std::filesystem::path& path,const std::string& text) {
    std::ofstream f(path,std::ios::binary);f.write(text.data(),std::streamsize(text.size()));assert(f.good());
}
static std::vector<std::string> load(const std::filesystem::path& path,std::string& error,Fault mode=Fault::None) {
    fault=mode;tracked_fd=-1;reads=closes=0;
    auto result=dkrr::ps5_mappings::read(path,&error);
    assert((open_flags&(O_NOFOLLOW|O_NONBLOCK))==(O_NOFOLLOW|O_NONBLOCK));
    assert(tracked_fd<0||closes==1);return result;
}
static void reset_import() {
    fault=Fault::None;additions=merges=closed_devices=refreshes=0;merged.clear();
    g_controller_devices={{1},{2}};g_controllers.fill(reinterpret_cast<void*>(1));g_gyro_controllers=g_controllers;
    g_platform_mutex.locks=0;observe_import=true;read_unlocked=false;
}
int main(int argc,char** argv) {
    assert(argc==2);namespace fs=std::filesystem;
    const fs::path root=argv[1];assert(fs::create_directory(root));std::string error;
    const auto normal=root/"mappings.txt";
    const std::string line="030000005e0400008e02000000000000,TestPad,a:b0,platform:Linux,";
    put(normal,"# comment\r\n\r\n"+line+"\r\n"+line);
    const std::vector<std::string> expected{line,line};
    assert(load(normal,error)==expected&&error.empty());
    assert(ReadMappingLines(normal)==expected);
    assert(load(normal,error,Fault::Interrupt)==expected&&error.empty()&&reads>=2);
    assert(load(normal,error,Fault::StatError).empty()&&!error.empty()&&reads==0);
    assert(load(normal,error,Fault::CloseError).empty()&&!error.empty());
    const auto long_line=root/"long.txt";put(long_line,std::string(dkrr::ps5_mappings::maximum_line,'a'));
    assert(load(long_line,error).size()==1&&error.empty());
    put(long_line,std::string(dkrr::ps5_mappings::maximum_line+1,'a'));
    assert(load(long_line,error).empty()&&error.find("too long")!=std::string::npos);
    const auto many=root/"many.txt";put(many,std::string(dkrr::ps5_mappings::maximum_lines+1,'\n'));
    assert(load(many,error).empty()&&error.find("too many")!=std::string::npos);
    const auto binary=root/"binary.txt";put(binary,std::string("a\0b",3));
    assert(load(binary,error).empty()&&!error.empty());
    const auto fifo=root/"replaced-selection.txt";assert(mkfifo(fifo.c_str(),0600)==0);
    assert(load(fifo,error).empty()&&!error.empty()&&reads==0);
    const auto directory=root/"folder";assert(fs::create_directory(directory));
    assert(load(directory,error).empty()&&!error.empty()&&reads==0);
    const auto link=root/"link.txt";fs::create_symlink(normal,link);
    assert(load(link,error).empty()&&!error.empty());
    const auto missing=root/"missing.txt";assert(load(missing,error).empty()&&!error.empty());
    const auto partial=root/"partial.txt";{std::ofstream f(partial);for(int n=0;n<5000;++n)f<<line<<'\n';}
    assert(load(partial,error,Fault::ReadError).empty()&&!error.empty()&&reads==2);
    const auto oversized=root/"large.txt";{std::ofstream f(oversized);const std::string row(1023,'a');for(std::uint64_t n=0;n<dkrr::ps5_mappings::maximum_bytes/1024+1;++n)f<<row<<'\n';}
    assert(load(oversized,error).empty()&&!error.empty()&&reads==0);
    assert(load(oversized,error,Fault::UnknownSize).empty()&&!error.empty()&&reads>0);
    fault=Fault::None;
    reset_import();std::string status;
    assert(dkr::runtime::platform::import_controller_mappings(normal,status));
    assert(read_unlocked&&additions==2&&merges==1&&merged==expected&&closed_devices==2&&refreshes==1);
    assert(g_controller_devices.empty()&&!g_platform_mutex.held);
    for(auto* p:g_controllers)assert(!p);
    reset_import();assert(!dkr::runtime::platform::import_controller_mappings(oversized,status));
    assert(read_unlocked&&!status.empty()&&!g_platform_mutex.locks&&!additions&&!merges&&g_controller_devices.size()==2);
    reset_import();fault=Fault::ReadError;assert(!dkr::runtime::platform::import_controller_mappings(partial,status));
    assert(read_unlocked&&!status.empty()&&!g_platform_mutex.locks&&!additions&&!merges);
    reset_import();dkr::runtime::platform::backend=dkr::runtime::platform::InputBackend::SDL3Native;
    assert(!dkr::runtime::platform::import_controller_mappings(normal,status));assert(!read_unlocked&&!additions&&!merges);
    observe_import=false;
    std::puts("PASS bounded native mapping reads: limits, nonblocking regular files, symlink refusal, EINTR retry, complete error refusal and closed descriptors");
    std::puts("PASS actual mapping import: USB I/O outside the input mutex, SDK/state updates inside it, no partial changes after read failure or oversized input");
}
