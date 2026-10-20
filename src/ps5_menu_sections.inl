// SPDX-License-Identifier: GPL-3.0-or-later
// Visible section buttons. L2/R2 change section from the page body too.
int DrawPs5SectionSelector(const char* id, const std::vector<const char*>& labels,
                          int current, float width) {
    if (labels.empty()) return -1;
    const int count=static_cast<int>(labels.size());
    const int active=std::clamp(current,0,count-1);
    const bool disabled=(ImGui::GetCurrentContext()->CurrentItemFlags&ImGuiItemFlags_Disabled)!=0;
    int picked=-1;
    if(!disabled&&!ImGui::IsAnyItemActive()&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)) {
        if(ImGui::IsKeyPressed(ImGuiKey_GamepadL2,false))picked=(active+count-1)%count;
        if(ImGui::IsKeyPressed(ImGuiKey_GamepadR2,false))picked=(active+1)%count;
    }
    ImGui::PushID(id);
    const float gap=8.0F;
    float minimum=72.0F;
    for(const char* label:labels)minimum=std::max(minimum,ImGui::CalcTextSize(label).x+24.0F);
    const int columns=std::clamp(static_cast<int>((width+gap)/(minimum+gap)),1,count);
    const float cell=std::max(1.0F,(width-gap*(columns-1))/columns);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,8.0F);
    for(int i=0;i<count;++i) {
        if(i%columns)ImGui::SameLine(0,gap);
        ImGui::PushID(i);
        if(i==active)ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if(ImGui::Button(labels[i],{cell,40.0F}))picked=i;
        if(i==active)ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return picked;
}
