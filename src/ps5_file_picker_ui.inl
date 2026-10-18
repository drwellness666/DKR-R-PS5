// SPDX-License-Identifier: GPL-3.0-or-later
// Drawn in the existing ImGui frame, with the current fonts and gamepad input.
void DrawPs5FilePicker() {
  using namespace dkrr::files;
  picker().mark_ui_thread();
  const auto state = picker().view();
  constexpr const char *popup = "CHOOSE A FILE###ps5-file-picker";
  static std::uint64_t opened = 0;
  if (!state.active) {
    if (opened && ImGui::BeginPopupModal(popup, nullptr,
                                         ImGuiWindowFlags_NoSavedSettings)) {
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
    opened = 0;
    return;
  }
  if (opened != state.generation) {
    ImGui::OpenPopup(popup);
    opened = state.generation;
  }
  const auto display = ImGui::GetIO().DisplaySize;
  const float scale = ImGui::GetFontSize() / 20.0F;
  ImGui::SetNextWindowSize({std::min(880.0F * scale, display.x * 0.90F),
                            std::min(620.0F * scale, display.y * 0.90F)},
                           ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(popup, nullptr,
                             ImGuiWindowFlags_NoSavedSettings)) {
    const bool back = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
                      ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (back) {
      picker().act(state.generation, Action::Cancel);
      ImGui::CloseCurrentPopup();
    }
    ImGui::TextUnformatted(state.mode == Mode::Open ? "Choose a file to import"
                           : state.mode == Mode::Save
                               ? "Choose an export folder"
                               : "Choose a folder");
    ImGui::TextWrapped("%s", state.directory.empty() ? "Locations"
                                                     : state.directory.c_str());
    if (state.mode == Mode::Save)
      ImGui::TextWrapped("File: %s (existing files are kept)",
                         state.filename.c_str());
    ImGui::BeginDisabled(state.loading);
    if (ImGui::Button("LOCATIONS"))
      picker().act(state.generation, Action::Roots);
    ImGui::SameLine();
    ImGui::BeginDisabled(state.directory.empty());
    if (ImGui::Button("UP ONE FOLDER"))
      picker().act(state.generation, Action::Up);
    ImGui::EndDisabled();
    if (state.mode != Mode::Open) {
      ImGui::SameLine();
      ImGui::BeginDisabled(state.directory.empty());
      if (ImGui::Button(state.mode == Mode::Save ? "EXPORT HERE"
                                                 : "USE THIS FOLDER"))
        picker().act(state.generation, Action::Accept);
      ImGui::EndDisabled();
    }
    if (!state.error.empty())
      ImGui::TextWrapped("%s", state.error.c_str());
    if (state.loading)
      ImGui::TextUnformatted("Reading folder...");
    const float footer = ImGui::GetFrameHeightWithSpacing() +
                         ImGui::GetTextLineHeightWithSpacing() * 2;
    if (ImGui::BeginChild("entries", {0, -footer}, ImGuiChildFlags_Border)) {
      if (state.entries.empty() && !state.loading)
        ImGui::TextUnformatted("No matching files or folders.");
      ImGuiListClipper clipper;
      clipper.Begin(static_cast<int>(state.entries.size()));
      while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
          const auto &entry = state.entries[static_cast<std::size_t>(i)];
          ImGui::PushID(i);
          // File names may contain ImGui's ## syntax. Use a fixed ID
          // and draw the actual name without parsing it as a label.
          const auto origin = ImGui::GetCursorScreenPos();
          const float height =
              ImGui::GetTextLineHeightWithSpacing() + 8.0F * scale;
          if (ImGui::Selectable("##entry", false,
                                ImGuiSelectableFlags_DontClosePopups,
                                {0, height}))
            picker().act(state.generation, Action::Enter, entry.name);
          std::string label = entry.name;
          if (state.directory.empty()) {
            if (label == "/data/dkrr")
              label = "Game files";
            else if (label.size() == 9 && label.rfind("/mnt/usb", 0) == 0)
              label = "USB drive " + std::to_string(label.back() - '0' + 1);
          }
          ImGui::GetWindowDrawList()->AddText(
              {origin.x + 6.0F * scale, origin.y + 4.0F * scale},
              ImGui::GetColorU32(ImGuiCol_Text),
              (std::string(entry.directory ? "[+] " : "    ") + label).c_str());
          ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::EndDisabled();
    if (ImGui::Button("CANCEL")) {
      picker().act(state.generation, Action::Cancel);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Cross: select   Circle: cancel");
    ImGui::TextUnformatted(
        "Connect a USB drive, then choose LOCATIONS to refresh.");
    ImGui::EndPopup();
  }
}
