// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_file_picker.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>
#include <sys/stat.h>
using namespace dkrr::files;
namespace fs = std::filesystem;
static std::atomic<int> read_fault = 0, close_fault = 0, throw_read = 0,
                        closed = 0;
static std::mutex pause_mutex;
static std::condition_variable pause_changed;
static std::string pause_path;
static bool pause_reached = false, pause_release = false;
extern "C" int __real_lstat(const char *, struct stat *);
extern "C" int __wrap_lstat(const char *path, struct stat *output) {
  {
    std::unique_lock lock(pause_mutex);
    if (!pause_path.empty() && pause_path == path) {
      pause_reached = true;
      pause_changed.notify_all();
      pause_changed.wait(lock, [] { return pause_release; });
    }
  }
  return __real_lstat(path, output);
}
extern "C" struct dirent *__real_readdir(DIR *);
extern "C" int __real_closedir(DIR *);
extern "C" struct dirent *__wrap_readdir(DIR *dir) {
  if (throw_read.exchange(0))
    throw std::bad_alloc();
  if (read_fault.exchange(0)) {
    errno = EIO;
    return nullptr;
  }
  return __real_readdir(dir);
}
extern "C" int __wrap_closedir(DIR *dir) {
  ++closed;
  const int rc = __real_closedir(dir);
  if (close_fault.exchange(0)) {
    errno = EIO;
    return -1;
  }
  return rc;
}
static View wait(Picker &p,
                 const std::function<bool(const View &)> &predicate) {
  for (int n = 0; n < 5000; ++n) {
    auto s = p.view();
    if (predicate(s))
      return s;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  assert(!"picker did not reach expected state");
  return {};
}
static auto start(Picker &p, Mode mode = Mode::Open,
                  const char *ext = "zip,rtz",
                  const char *name = "export.bin") {
  const auto previous = p.view().generation;
  auto result = std::async(std::launch::async, [&p, mode, ext, name] {
    return p.run(mode, ext, name);
  });
  wait(p, [&](const View &v) {
    return v.active && !v.loading && v.generation > previous;
  });
  return result;
}
static View enter(Picker &p, const std::string &name,
                  const std::string &expected) {
  const auto state = p.view();
  assert(p.act(state.generation, Action::Enter, name));
  return wait(
      p, [&](const View &v) { return v.directory == expected && !v.loading; });
}
static Result finish(std::future<Result> &f) {
  assert(f.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  return f.get();
}
int main(int argc, char **argv) {
  assert(argc == 2);
  const fs::path base = argv[1], root = base / "usb",
                 outside = base / "outside";
  fs::create_directories(root / "nested");
  fs::create_directories(outside);
  fs::create_directories(base / "missing-usb");
  std::ofstream(outside / "secret.zip") << "outside";
  for (const char *name :
       {"pack.ZIP", "literal##file.rtz", "ignored.bin", "export.bin"})
    std::ofstream(root / name) << name;
  fs::create_directory_symlink(outside, root / "linked-folder");
  fs::create_symlink(outside / "secret.zip", root / "linked.zip");
  Picker p({root.string(), (base / "missing-usb").string()});
  auto a = start(p);
  auto v = enter(p, root.string(), root.string());
  assert(v.entries.size() == 3 && v.entries.front().directory);
  assert(!p.run(Mode::Folder, "", "").error.empty());
  assert(!p.act(v.generation - 1, Action::Cancel));
  assert(p.act(v.generation, Action::Enter, "linked.zip"));
  assert(a.wait_for(std::chrono::milliseconds(20)) ==
         std::future_status::timeout);
  // The intentionally rejected entry command must leave the picker open.
  for (int n = 0; n < 500 && !p.act(v.generation, Action::Cancel); ++n)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  assert(finish(a).path.empty());
  a = start(p);
  v = enter(p, root.string(), root.string());
  assert(p.act(v.generation, Action::Enter, "literal##file.rtz"));
  assert(finish(a).path == (root / "literal##file.rtz").string());
  a = start(p, Mode::Save);
  v = enter(p, root.string(), root.string());
  assert(v.entries.size() == 1 && v.entries.front().directory);
  assert(p.act(v.generation, Action::Accept));
  assert(finish(a).path == (root / "export-1.bin").string());
  assert(fs::file_size(root / "export.bin") > 0);
  a = start(p, Mode::Folder, "", "");
  v = enter(p, root.string(), root.string());
  v = enter(p, "nested", (root / "nested").string());
  assert(p.act(v.generation, Action::Up));
  v = wait(p, [&](const View &s) {
    return s.directory == root.string() && !s.loading;
  });
  assert(p.act(v.generation, Action::Up));
  v = wait(p, [](const View &s) { return s.directory.empty() && !s.loading; });
  assert(v.entries.size() == 2);
  fs::remove(base / "missing-usb");
  v = enter(p, (base / "missing-usb").string(),
            (base / "missing-usb").string());
  assert(!v.error.empty());
  assert(p.act(v.generation, Action::Roots));
  wait(p, [](const View &s) { return s.directory.empty(); });
  v = enter(p, root.string(), root.string());
  assert(p.act(v.generation, Action::Accept));
  assert(finish(a).path == root.string());
  for (bool close : {false, true}) {
    a = start(p);
    if (close)
      close_fault = 1;
    else
      read_fault = 1;
    v = enter(p, root.string(), root.string());
    assert(v.entries.empty() && !v.error.empty());
    assert(p.act(v.generation, Action::Cancel));
    assert(finish(a).path.empty());
  }
  // Cancel wins over a queued navigation command, including rapid inputs.
  for (unsigned iteration = 0; iteration < 100; ++iteration) {
    a = start(p);
    const auto generation = p.view().generation;
    assert(p.act(generation, Action::Roots));
    assert(p.act(generation, Action::Cancel));
    assert(finish(a).path.empty());
  }
  // Cancel must also win while a chosen export destination is being checked.
  a = start(p, Mode::Save);
  v = enter(p, root.string(), root.string());
  {
    std::lock_guard lock(pause_mutex);
    pause_path = (root / "export-1.bin").string();
  }
  assert(p.act(v.generation, Action::Accept));
  {
    std::unique_lock lock(pause_mutex);
    assert(pause_changed.wait_for(lock, std::chrono::seconds(3),
                                  [] { return pause_reached; }));
  }
  assert(p.act(v.generation, Action::Cancel));
  {
    std::lock_guard lock(pause_mutex);
    pause_release = true;
    pause_changed.notify_all();
  }
  assert(finish(a).path.empty());
  {
    std::lock_guard lock(pause_mutex);
    pause_path.clear();
  }
  // Model an allocation exception during enumeration, with a directory already
  // open. The handle must close and a subsequent request must remain usable.
  a = start(p);
  const int before_exception = closed;
  throw_read = 1;
  assert(p.act(p.view().generation, Action::Enter, root.string()));
  assert(a.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  bool threw = false;
  try {
    a.get();
  } catch (const std::bad_alloc &) {
    threw = true;
  }
  assert(threw && !p.view().active && closed == before_exception + 1);
  a = start(p);
  v = enter(p, root.string(), root.string());
  fs::remove(root / "pack.ZIP");
  assert(p.act(v.generation, Action::Enter, "pack.ZIP"));
  v = wait(p, [](const View &s) { return !s.error.empty(); });
  assert(v.active);
  p.shutdown();
  assert(finish(a).path.empty());
  assert(!p.run(Mode::Open, "", "").error.empty());
  Picker ui({root.string()});
  ui.mark_ui_thread();
  assert(!ui.run(Mode::Open, "", "").error.empty());
  Picker invalid({root.string()});
  assert(!invalid.run(Mode::Save, "bin", "../escape.bin").error.empty());
  std::puts("PASS native picker navigation, filtering, literal labels, root "
            "boundaries, symlinks, unavailable USB, non-overwriting exports, "
            "I/O failure, exception resource cleanup, rapid cancellation and "
            "shutdown");
}
