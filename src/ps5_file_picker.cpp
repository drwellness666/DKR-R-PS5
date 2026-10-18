// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_file_picker.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <functional>
#include <memory>
#include <sys/stat.h>
#include <utility>

namespace dkrr::files {
namespace {
constexpr std::size_t maximum_path = 768, maximum_entries = 1024;
bool basename(const std::string &name) {
  return !name.empty() && name.size() <= 255 && name != "." && name != ".." &&
         name.find_first_of("/\\") == std::string::npos &&
         std::none_of(name.begin(), name.end(),
                      [](unsigned char c) { return c < 32 || c == 127; });
}
bool ordinary(const std::string &path, bool directory) {
  if (path.empty() || path.front() != '/' || path.size() > maximum_path)
    return false;
  // Check every ancestor: checking only the leaf would allow USB links to
  // escape the selected storage root. No canonicalization that follows links.
  std::size_t offset = 1;
  for (;;) {
    const auto end = path.find('/', offset);
    const auto part =
        path.substr(offset, end == std::string::npos ? end : end - offset);
    if (part.empty() || part == "." || part == "..")
      return false;
    struct stat info{};
    if (lstat(path.substr(0, end).c_str(), &info) != 0)
      return false;
    if (end == std::string::npos)
      return directory ? S_ISDIR(info.st_mode) : S_ISREG(info.st_mode);
    if (!S_ISDIR(info.st_mode))
      return false;
    offset = end + 1;
  }
}
std::string lower(std::string value) {
  for (char &c : value)
    c = char(std::tolower(static_cast<unsigned char>(c)));
  return value;
}
bool matches(const std::string &name, const std::string &extensions) {
  if (extensions.empty())
    return true;
  const auto dot = name.rfind('.');
  if (dot == std::string::npos)
    return false;
  const auto extension = lower(name.substr(dot + 1));
  std::size_t start = 0;
  do {
    const auto end = extensions.find(',', start);
    if (extension ==
        extensions.substr(start, end == std::string::npos ? end : end - start))
      return true;
    if (end == std::string::npos)
      break;
    start = end + 1;
  } while (start < extensions.size());
  return false;
}
std::string safe_export(const std::string &directory, const std::string &name) {
  if (!basename(name) || !ordinary(directory, true))
    return {};
  const auto dot = name.rfind('.');
  const auto stem = dot == std::string::npos ? name : name.substr(0, dot);
  const auto suffix =
      dot == std::string::npos ? std::string{} : name.substr(dot);
  for (unsigned n = 0; n < 128; ++n) {
    const auto path =
        directory + "/" + stem + (n ? "-" + std::to_string(n) : "") + suffix;
    if (path.size() > maximum_path)
      return {};
    struct stat info{};
    errno = 0;
    if (lstat(path.c_str(), &info) != 0) {
      if (errno == ENOENT)
        return path;
      return {};
    }
  }
  return {};
}
std::vector<Entry> list(const std::string &directory, Mode mode,
                        const std::string &extensions, std::string &error,
                        const std::function<bool()> &cancelled) {
  std::vector<Entry> result;
  if (!ordinary(directory, true)) {
    error = "This folder is unavailable. Check the USB drive or choose another "
            "location.";
    return result;
  }
  std::unique_ptr<DIR, int (*)(DIR *)> dir(opendir(directory.c_str()), closedir);
  if (!dir) {
    error = "This folder could not be opened.";
    return result;
  }
  bool failed = false;
  std::size_t examined = 0;
  for (;;) {
    if (cancelled())
      break;
    errno = 0;
    const auto *entry = readdir(dir.get());
    if (!entry) {
      if (errno)
        failed = true;
      break;
    }
    if (++examined > 4096) {
      error = "This folder is too large. Put the files you need in a smaller "
              "folder.";
      break;
    }
    const std::string name(entry->d_name);
    if (!basename(name))
      continue;
    const auto path = directory + "/" + name;
    const bool is_dir = ordinary(path, true);
    if (!is_dir && (mode != Mode::Open || !matches(name, extensions) ||
                    !ordinary(path, false)))
      continue;
    if (result.size() == maximum_entries) {
      error =
          "This folder has too many matching entries. Use a smaller folder.";
      break;
    }
    result.push_back({name, is_dir});
  }
  if (closedir(dir.release()) != 0)
    failed = true;
  if (failed) {
    result.clear();
    error = "The folder could not be fully read. Reconnect the USB drive and "
            "try again.";
  }
  std::sort(result.begin(), result.end(), [](const Entry &a, const Entry &b) {
    return a.directory != b.directory ? a.directory > b.directory
                                      : lower(a.name) < lower(b.name);
  });
  return result;
}
} // namespace
Picker::Picker(std::vector<std::string> roots) : roots_(std::move(roots)) {}
Result Picker::run(Mode mode, const std::string &extensions,
                   const std::string &filename) {
  std::unique_lock lock(mutex_);
  if (stopped_)
    return {{}, "The file picker is closing."};
  if (ui_thread_ == std::this_thread::get_id())
    return {{}, "The file picker must run on the dialog worker."};
  if (view_.active)
    return {{}, "Another file picker is already open."};
  if (extensions.size() > 256 || (mode == Mode::Save && !basename(filename)))
    return {{}, "The requested file name or filter is invalid."};
  const auto generation = view_.generation + 1;
  try {
    view_ = {};
    view_.active = true;
    view_.generation = generation;
    view_.mode = mode;
    view_.filename = filename;
    pending_ = false;
    auto show_roots = [&] {
      view_.loading = true;
      lock.unlock();
      std::vector<Entry> available;
      for (const auto &root : roots_)
        if (ordinary(root, true))
          available.push_back({root, true});
      lock.lock();
      view_.loading = false;
      view_.directory.clear();
      view_.entries = std::move(available);
      view_.error = view_.entries.empty()
                        ? "No storage locations are available. Connect a USB "
                          "drive and choose Locations again."
                        : "";
    };
    show_roots();
    auto done = [&](Result result) {
      if (pending_ && action_ == Action::Cancel)
        result = {};
      view_.active = false;
      view_.loading = false;
      view_.entries.clear();
      pending_ = false;
      return result;
    };
    for (;;) {
      changed_.wait(lock, [&] { return stopped_ || pending_; });
      if (stopped_)
        return done({});
      const auto action = action_;
      const auto name = std::move(name_);
      pending_ = false;
      if (action == Action::Cancel)
        return done({});
      if (action == Action::Roots) {
        show_roots();
        continue;
      }
      if (action == Action::Accept && !view_.directory.empty() &&
          mode != Mode::Open) {
        const auto directory = view_.directory;
        lock.unlock();
        const auto selected =
            mode == Mode::Folder
                ? (ordinary(directory, true) ? directory : std::string{})
                : safe_export(directory, filename);
        lock.lock();
        if (stopped_)
          return done({});
        if (!selected.empty())
          return done({selected, {}});
        view_.error = "The destination is unavailable or a free export name "
                      "could not be found.";
        continue;
      }
      std::string next;
      if (action == Action::Up) {
        if (view_.directory.empty())
          continue;
        if (std::find(roots_.begin(), roots_.end(), view_.directory) !=
            roots_.end()) {
          show_roots();
          continue;
        }
        next = view_.directory.substr(0, view_.directory.rfind('/'));
      } else if (action == Action::Enter) {
        const auto entry =
            std::find_if(view_.entries.begin(), view_.entries.end(),
                         [&](const Entry &e) { return e.name == name; });
        if (entry == view_.entries.end())
          continue;
        if (!entry->directory) {
          const auto selected = view_.directory + "/" + name;
          lock.unlock();
          const bool valid = ordinary(selected, false);
          lock.lock();
          if (stopped_)
            return done({});
          if (valid)
            return done({selected, {}});
          view_.error = "This file is no longer available.";
          continue;
        }
        next = view_.directory.empty() ? name : view_.directory + "/" + name;
      } else
        continue;
      view_.loading = true;
      view_.error.clear();
      lock.unlock();
      std::string error;
      auto entries = list(next, mode, lower(extensions), error, [&] {
        std::lock_guard guard(mutex_);
        return stopped_ || (pending_ && action_ == Action::Cancel);
      });
      lock.lock();
      if (stopped_)
        return done({});
      view_.loading = false;
      view_.directory = std::move(next);
      view_.entries = std::move(entries);
      view_.error = std::move(error);
    }
  } catch (...) {
    if (!lock.owns_lock())
      lock.lock();
    view_.active = false;
    view_.loading = false;
    view_.entries.clear();
    pending_ = false;
    throw;
  }
}
View Picker::view() {
  std::lock_guard lock(mutex_);
  if (!view_.active) {
    View idle;
    idle.generation = view_.generation;
    return idle;
  }
  return view_;
}
bool Picker::act(std::uint64_t generation, Action action,
                 const std::string &name) {
  std::lock_guard lock(mutex_);
  if (!view_.active || view_.generation != generation ||
      (pending_ && action != Action::Cancel) ||
      (view_.loading && action != Action::Cancel))
    return false;
  action_ = action;
  name_ = name;
  pending_ = true;
  changed_.notify_one();
  return true;
}
void Picker::shutdown() {
  std::lock_guard lock(mutex_);
  stopped_ = true;
  changed_.notify_all();
}
void Picker::mark_ui_thread() {
  std::lock_guard lock(mutex_);
  ui_thread_ = std::this_thread::get_id();
}
Picker &picker() {
  static Picker instance({"/data/dkrr", "/mnt/usb0", "/mnt/usb1", "/mnt/usb2",
                          "/mnt/usb3", "/mnt/usb4", "/mnt/usb5", "/mnt/usb6",
                          "/mnt/usb7"});
  return instance;
}
} // namespace dkrr::files
extern "C" void dkrr_ps5_cancel_file_picker() {
  dkrr::files::picker().shutdown();
}
namespace {
thread_local std::string picker_error;
}
extern "C" const char *dkrr_ps5_file_picker_error() {
  return picker_error.c_str();
}
extern "C" int dkrr_ps5_choose_file(int mode, char **output,
                                    const char *extensions, const char *name) {
  if (!output)
    return 0;
  *output = nullptr;
  picker_error.clear();
  if (mode < 0 || mode > 2) {
    picker_error = "Invalid file picker request.";
    return 0;
  }
  try {
    auto result = dkrr::files::picker().run(
        static_cast<dkrr::files::Mode>(mode), extensions ? extensions : "",
        name ? name : "");
    if (!result.error.empty()) {
      picker_error = std::move(result.error);
      return 0;
    }
    if (result.path.empty())
      return 2;
    *output = static_cast<char *>(std::malloc(result.path.size() + 1));
    if (!*output) {
      picker_error = "There is not enough memory for this file selection.";
      return 0;
    }
    std::memcpy(*output, result.path.c_str(), result.path.size() + 1);
    return 1;
  } catch (...) {
    picker_error = "The file picker could not complete this request.";
    return 0;
  }
}
