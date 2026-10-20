// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <mutex>
#include <nfd.h>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
static std::string g_track_import_status, g_texture_pack_status;
static std::string observed_filter, observed_name;
static std::thread::id selection_thread;
static nfdresult_t selection_result = NFD_OKAY;
extern "C" {
nfdresult_t NFD_Init() { return NFD_OKAY; }
void NFD_Quit() {}
const char *NFD_GetError() { return "Test storage error"; }
void NFD_FreePathU8(nfdu8char_t *p) { std::free(p); }
nfdresult_t NFD_OpenDialogU8(nfdu8char_t **p, const nfdu8filteritem_t *filters,
                             nfdfiltersize_t count, const nfdu8char_t *) {
  selection_thread = std::this_thread::get_id();
  assert(count == 1);
  observed_filter = filters[0].spec;
  *p = selection_result == NFD_OKAY ? strdup("/tmp/chosen.zip") : nullptr;
  return selection_result;
}
nfdresult_t NFD_SaveDialogU8(nfdu8char_t **p, const nfdu8filteritem_t *filters,
                             nfdfiltersize_t count, const nfdu8char_t *initial,
                             const nfdu8char_t *name) {
  observed_name = name;
  return NFD_OpenDialogU8(p, filters, count, initial);
}
}
// Replace only thread construction in the extracted production queue. The
// queue state, exception handling and retry remain the actual implementation.
static bool fail_next_thread = false;
struct FixtureDialogThread {
  std::jthread thread;
  FixtureDialogThread() = default;
  FixtureDialogThread(FixtureDialogThread&&) = default;
  FixtureDialogThread& operator=(FixtureDialogThread&&) = default;
  template<class Function> explicit FixtureDialogThread(Function function) {
    if (fail_next_thread) {
      fail_next_thread = false;
      throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
    }
    thread = std::jthread(std::move(function));
  }
  bool joinable() const { return thread.joinable(); }
  void join() { thread.join(); }
};
static bool shared_busy=false;
bool SharedSaveBusy(){return shared_busy;}
#include "file_actions.inc"
static void await_worker() {
  for (int i = 0; i < 5000; ++i) {
    {
      std::lock_guard lock(g_dialog_job_mutex);
      if (!g_dialog_job_running)
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  assert(false);
}
int main() {
  shared_busy=true;assert(DialogJobRunning());shared_busy=false;
  const auto main = std::this_thread::get_id();
  std::string status;
  unsigned completions = 0;
  nfdfilteritem_t initial_filter{"Archive", "zip"};
  fail_next_thread = true;
  assert(!ChooseFileForAction(&initial_filter, 1, nullptr, status,
                             [](const std::filesystem::path&) { return true; }));
  assert(!DialogJobRunning() && !status.empty());
  status.clear();
  auto action = [&](const std::filesystem::path &p) {
    assert(std::this_thread::get_id() == main && p == "/tmp/chosen.zip");
    ++completions;
    return true;
  };
  {
    std::string description = "Texture archive", extension = "zip,rtz";
    nfdfilteritem_t filter{description.c_str(), extension.c_str()};
    assert(ChooseFileForAction(&filter, 1, nullptr, status, action));
  }
  await_worker();
  assert(completions == 0 && selection_thread != main &&
         observed_filter == "zip,rtz");
  nfdfilteritem_t filter{"Archive", "zip"};
  // A new action must not replace a selected file whose completion has not yet
  // reached the UI thread. The previous queue discarded that first action.
  assert(DialogJobRunning());
  assert(!ChooseFileForAction(&filter, 1, nullptr, status, action));
  PumpDialogJob();
  assert(completions == 1 && !DialogJobRunning());
  assert(ChooseFileForAction(&filter, 1, "my-export.zip", status, action));
  await_worker();
  PumpDialogJob();
  assert(completions == 2 && observed_name == "my-export.zip");
  selection_result = NFD_ERROR;
  assert(ChooseFileForAction(&filter, 1, nullptr, status, action));
  await_worker();
  PumpDialogJob();
  assert(status == "Test storage error" && completions == 2);
  selection_result = NFD_CANCEL;
  status.clear();
  assert(ChooseFileForAction(&filter, 1, nullptr, status, action));
  await_worker();
  PumpDialogJob();
  assert(status.empty() && completions == 2);
  if (g_dialog_job_worker.joinable())
    g_dialog_job_worker.join();
  std::puts("PASS actual picker action helper: worker isolation, copied "
            "filters, pending completion preservation, UI-thread actions, save "
            "names, errors, cancellation and thread-start retry");
}
