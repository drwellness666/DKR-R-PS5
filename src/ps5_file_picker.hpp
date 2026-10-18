// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace dkrr::files {
enum class Mode { Open, Save, Folder };
enum class Action { Enter, Up, Roots, Accept, Cancel };
struct Entry {
  std::string name;
  bool directory = false;
};
struct View {
  bool active = false, loading = false;
  std::uint64_t generation = 0;
  Mode mode = Mode::Open;
  std::string directory, filename, error;
  std::vector<Entry> entries;
};
struct Result {
  std::string path, error;
};
// Only the dialog worker touches storage. The UI reads a bounded snapshot and
// posts navigation commands, so a slow USB device never stalls presentation.
class Picker {
public:
  explicit Picker(std::vector<std::string> roots);
  Result run(Mode mode, const std::string &extensions, const std::string &name);
  View view();
  bool act(std::uint64_t generation, Action action,
           const std::string &name = {});
  void shutdown();
  void mark_ui_thread();

private:
  std::vector<std::string> roots_;
  std::mutex mutex_;
  std::condition_variable changed_;
  View view_;
  bool stopped_ = false, pending_ = false;
  Action action_ = Action::Cancel;
  std::string name_;
  std::thread::id ui_thread_;
};
Picker &picker();
} // namespace dkrr::files

extern "C" void dkrr_ps5_cancel_file_picker();
extern "C" int dkrr_ps5_choose_file(int mode, char **output,
                                    const char *extensions, const char *name);
extern "C" const char *dkrr_ps5_file_picker_error();
