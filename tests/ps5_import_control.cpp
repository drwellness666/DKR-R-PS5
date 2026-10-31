// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_import_control.hpp"
#include "ps5_embedded_importer.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sys/stat.h>
#include <thread>
namespace dkr::mods {struct Error:std::runtime_error{using std::runtime_error::runtime_error;};}
static std::atomic_int calls{},inside{},maximum{};static std::atomic_bool slow{};
static int execute(const std::filesystem::path&) {
 ++calls;const int active=++inside;maximum.store(std::max(maximum.load(),active));
 struct Exit{~Exit(){--inside;}} exit;
 if(slow)for(int i=0;i<20;++i){std::this_thread::sleep_for(std::chrono::milliseconds(1));dkrr::imports::checkpoint();}
 return 0;
}
#include "ps5_embedded_importer.inl"
template<class F>static void rejects(F&&f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed&&dkrr::imports::active==nullptr);}
int main(){using namespace dkrr::imports;using dkr::mods::WorkerOutcome;namespace fs=std::filesystem;
 const fs::path root=DKRR_NATIVE_IMPORT_ROOT;const auto request=root/"profile-us-v77/mods/legacy/jobs/job/request.json";fs::create_directories(request.parent_path());std::ofstream(request)<<"{}";
 const std::function<bool()> yes=[] {return true;},no=[] {return false;};
 assert(run("/app0/eboot.bin",request,yes,std::chrono::seconds(1)).outcome==WorkerOutcome::Completed);
 assert(run("/app0/eboot.bin",request,no,std::chrono::seconds(1)).outcome==WorkerOutcome::Cancelled&&calls==1);
 rejects([&]{run("/different.elf",request,yes,std::chrono::seconds(1));});rejects([&]{run("/app0/eboot.bin",root/"request.json",yes,std::chrono::seconds(1));});
 slow=true;assert(run("/app0/eboot.bin",request,yes,std::chrono::milliseconds(1)).outcome==WorkerOutcome::TimedOut&&inside==0);
 std::thread first([&]{assert(run("/app0/eboot.bin",request,yes,std::chrono::seconds(2)).outcome==WorkerOutcome::Completed);});std::thread second([&]{assert(run("/app0/eboot.bin",request,yes,std::chrono::seconds(2)).outcome==WorkerOutcome::Completed);});first.join();second.join();assert(maximum==1&&inside==0);
 fs::remove(request);const auto target=root/"target";std::ofstream(target)<<"{}";fs::create_symlink(target,request);rejects([&]{run("/app0/eboot.bin",request,yes,std::chrono::seconds(1));});
 puts("PASS embedded import ownership: no executable launch, ordinary private request only, cancellation, timeout, one serial worker, thread-local scope retirement and symlink refusal");
}
