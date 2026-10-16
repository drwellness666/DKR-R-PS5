// SPDX-License-Identifier: GPL-3.0-or-later
// A single focus target replaces a potentially overflowing horizontal tab row.
// The popup uses native ImGui gamepad navigation and restores focus on close.
int DrawPs5SectionSelector(const char* id, const std::vector<const char*>& labels,
                          int current, float width) {
    if (labels.empty()) return -1;
    const int active=std::clamp(current,0,static_cast<int>(labels.size())-1);
    int picked=-1;
    ImGui::PushID(id);
    ImGui::TextUnformatted("SECTION");
    ImGui::SetNextItemWidth(std::max(width,1.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{14.0F,12.0F});
    if (ImGui::BeginCombo("##section",labels[active])) {
        for (int i=0;i<static_cast<int>(labels.size());++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(labels[i],i==active,0,{0.0F,32.0F})) picked=i;
            if (i==active) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return picked;
}
