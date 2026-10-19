// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_file_picker.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <nfd.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <thread>
#include <atomic>
namespace dkr::runtime::ui { void toggle_overlay(); }
namespace dkr::runtime::hud::editor { bool active(){return false;} void request_back(){} }
static std::atomic_bool g_overlay_visible{true},g_overlay_focus_requested{false};
static std::atomic_int g_overlay_last_rendered_page{-1},g_overlay_sidebar_selection{0},g_overlay_sidebar_action{0};
static int g_overlay_page=0;
#define __PROSPERO__ 1
#include "overlay_toggle.inc"
#undef __PROSPERO__
#include "ps5_file_picker_ui.inl"
static void frame() {
  ImGui::NewFrame();
  DrawPs5FilePicker();
  ImGui::Render();
}
static void key(ImGuiKey key) {
  auto &io = ImGui::GetIO();
  io.AddKeyEvent(key, true);
  frame();
  frame();
  io.AddKeyEvent(key, false);
  frame();
  frame();
}
static auto request(dkrr::files::Mode mode) {
  auto &picker = dkrr::files::picker();
  const auto generation = picker.view().generation;
  auto future = std::async(std::launch::async, [mode] {
    assert(NFD_Init() == NFD_OKAY);
    const nfdfilteritem_t filters[] = {{"Archive", "zip"}};
    nfdu8char_t *path = nullptr;
    const auto code =
        mode == dkrr::files::Mode::Open
            ? NFD_OpenDialogU8(&path, filters, 1, nullptr)
        : mode == dkrr::files::Mode::Save
            ? NFD_SaveDialogU8(&path, filters, 1, nullptr, "dkr-backup.bin")
            : NFD_PickFolderU8(&path, nullptr);
    dkrr::files::Result result;
    if (code == NFD_OKAY) {
      assert(path);
      result.path = path;
      NFD_FreePathU8(path);
    } else {
      assert(path == nullptr && code == NFD_CANCEL);
    }
    NFD_Quit();
    return result;
  });
  for (int i = 0; i < 2000; ++i) {
    auto view = picker.view();
    if (view.active && !view.loading && view.generation > generation)
      return future;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  assert(!"request did not start");
  return future;
}
static void closed(std::future<dkrr::files::Result> &future) {
  assert(future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  assert(future.get().path.empty());
  frame();
  frame();
  assert(!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId |
                                          ImGuiPopupFlags_AnyPopupLevel));
}
int main() {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = {1920, 1080};
  io.DeltaTime = 1.0F / 60;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  frame();
  auto f = request(dkrr::files::Mode::Open);
  frame();
  frame();
  assert(GImGui->OpenPopupStack.Size == 1);
  const auto *modal = GImGui->OpenPopupStack.back().Window;
  assert(modal && modal->Size.x <= io.DisplaySize.x * 0.91F &&
         modal->Size.y <= io.DisplaySize.y * 0.91F);
  key(ImGuiKey_GamepadDpadDown);
  key(ImGuiKey_GamepadDpadDown);
  assert(GImGui->NavWindow && GImGui->NavWindow->RootWindow == modal);
  key(ImGuiKey_GamepadFaceRight);
  closed(f);
  // A second picker must reopen, and worker-side cancellation must also remove
  // the modal. Otherwise all later touchpad-menu controls remain blocked.
  f = request(dkrr::files::Mode::Save);
  frame();
  frame();
  assert(GImGui->OpenPopupStack.Size == 1);
  auto state = dkrr::files::picker().view();
  assert(
      dkrr::files::picker().act(state.generation, dkrr::files::Action::Cancel));
  closed(f);
  // Exercise the real runtime toggle, used by Touchpad and Escape. It must
  // dismiss this modal while keeping race input blocked, then resume normally.
  f=request(dkrr::files::Mode::Open);frame();frame();
  dkr::runtime::ui::toggle_overlay();assert(g_overlay_visible.load());closed(f);
  dkr::runtime::ui::toggle_overlay();assert(!g_overlay_visible.load()&&g_overlay_page==0);
  io.DisplaySize = {640, 360};
  f = request(dkrr::files::Mode::Folder);
  frame();
  frame();
  modal = GImGui->OpenPopupStack.back().Window;
  assert(modal->Size.x <= 640 * 0.91F && modal->Size.y <= 360 * 0.91F);
  key(ImGuiKey_Escape);
  closed(f);
  dkrr::files::picker().shutdown();
  frame();
  ImGui::DestroyContext();
  std::puts(
      "PASS actual NFD C ABI and real ImGui picker modal, gamepad focus, "
      "Circle/Escape "
      "cancellation, worker completion, reopening, Touchpad lifecycle and small-screen bounds");
}
