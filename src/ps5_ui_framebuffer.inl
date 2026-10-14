// SPDX-License-Identifier: GPL-3.0-or-later
// SDL keeps a 1920x1080 logical game window while the native Vulkan surface
// may use 1440p or 2160p. ImGui's Vulkan backend already scales its viewport
// and clips from FramebufferScale; preserve logical layout/navigation/fonts.
const auto ps5_ui_output = dkrr_ps5_display_status();
ImGuiIO& ps5_ui_io = ImGui::GetIO();
if (ps5_ui_output.width && ps5_ui_output.height &&
    std::isfinite(ps5_ui_io.DisplaySize.x) && std::isfinite(ps5_ui_io.DisplaySize.y) &&
    ps5_ui_io.DisplaySize.x > 0.0F && ps5_ui_io.DisplaySize.y > 0.0F) {
    const float ps5_ui_scale_x = float(ps5_ui_output.width) / ps5_ui_io.DisplaySize.x;
    const float ps5_ui_scale_y = float(ps5_ui_output.height) / ps5_ui_io.DisplaySize.y;
    if (ps5_ui_scale_x >= 0.25F && ps5_ui_scale_x <= 4.0F &&
        ps5_ui_scale_y >= 0.25F && ps5_ui_scale_y <= 4.0F)
        ps5_ui_io.DisplayFramebufferScale = {ps5_ui_scale_x, ps5_ui_scale_y};
}
