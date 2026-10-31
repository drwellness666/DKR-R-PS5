// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "legacy_mod_process.hpp"
namespace dkrr::imports {
dkr::mods::WorkerResult run(const std::filesystem::path& executable,const std::filesystem::path& request,const std::function<bool()>& keep_running,std::chrono::milliseconds timeout);
}
