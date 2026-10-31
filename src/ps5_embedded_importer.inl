// SPDX-License-Identifier: GPL-3.0-or-later
// The surrounding translation unit includes the unmodified upstream request
// dispatcher, with progress/file/decoder checkpoints. No package code executes.
#ifndef DKRR_NATIVE_IMPORT_ROOT
#define DKRR_NATIVE_IMPORT_ROOT "/data/dkrr"
#endif
namespace dkrr::imports {
dkr::mods::WorkerResult run(const std::filesystem::path& executable,const std::filesystem::path& request,const std::function<bool()>& keep_running,std::chrono::milliseconds timeout) {
    using namespace dkr::mods;
    if(executable!="/app0/eboot.bin"||!request.is_absolute()||request.filename()!="request.json"||timeout.count()<=0)
        throw Error("Invalid native import request.");
    const auto text=request.generic_string();
    if(!text.starts_with(DKRR_NATIVE_IMPORT_ROOT "/profile-us-v77/")&&!text.starts_with(DKRR_NATIVE_IMPORT_ROOT "/profile-us-v80/"))throw Error("Import requests must remain in the private DKR-R profile.");
    auto path=std::filesystem::path("/");
    for(const auto& part:request.relative_path()) {
        if(part=="."||part=="..")throw Error("Invalid native import path.");
        path/=part;struct stat info{};
        if(lstat(path.c_str(),&info)<0||(path==request?!S_ISREG(info.st_mode):!S_ISDIR(info.st_mode)))throw Error("Native import paths must be ordinary files and directories.");
    }
    Scope scope(keep_running,std::min(timeout,std::chrono::milliseconds(180000)));
    static std::mutex import_mutex;std::unique_lock lock(import_mutex,std::defer_lock);
    try {
        while(!lock.try_lock()){checkpoint();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
        checkpoint();const int result=execute(request);checkpoint();
        return {result?WorkerOutcome::Failed:WorkerOutcome::Completed,unsigned(result)};
    }catch(const std::exception&) {
        if(scope.control.cancelled)return {WorkerOutcome::Cancelled,0};
        if(scope.control.timed_out)return {WorkerOutcome::TimedOut,0};
        throw;
    }
}
}
