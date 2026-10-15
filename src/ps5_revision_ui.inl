// SPDX-License-Identifier: GPL-3.0-or-later
// Native pause page: visual cards select only the next launch, never live saves.
static std::string revision_choice_status;
static struct dkrr_ps5_revision_state cached_revisions{};
static double next_revision_check=0;
static ImGuiContext* revision_context=nullptr;
const double revision_now=ImGui::GetTime();
if(revision_context!=ImGui::GetCurrentContext()||revision_now>=next_revision_check) {
    revision_context=ImGui::GetCurrentContext();
    cached_revisions=dkrr_ps5_revision_state();next_revision_check=revision_now+1.0;
}
const auto revisions=cached_revisions;
// The caller also uses content_width for Return to the race. Window availability
// includes extra panel padding and must not determine the card boundary.
const float version_width=std::max(content_width,1.0F);
ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+version_width);
ImGui::TextUnformatted("GAME VERSION");
ImGui::Text("Playing: US %s",revisions.active==80?"1.1 - Rev A":"1.0 - Original");
if(revisions.preferred!=revisions.active)
    ImGui::Text("Next start: US %s",revisions.preferred==80?"1.1 - Rev A":"1.0 - Original");
else ImGui::TextUnformatted("Your current version is also selected for the next start.");
ImGui::Dummy({0.0F,10.0F});
for(unsigned choice:{77U,80U}) {
    const unsigned flag=choice==77?1U:2U;
    const bool compiled=(revisions.compiled_mask&flag)!=0;
    const bool ready=compiled&&(revisions.available_mask&flag);
    const bool selected=revisions.preferred==choice;
    const bool playing=revisions.active==choice;
    ImGui::BeginDisabled(!ready);
    const bool clicked=ImGui::InvisibleButton(choice==77?"##version-us10":"##version-us11",{version_width,88.0F});
    const ImVec2 at=ImGui::GetItemRectMin();
    const ImVec2 end=ImGui::GetItemRectMax();
    auto* draw=ImGui::GetWindowDrawList();
    const bool focused=ImGui::IsItemFocused(), hovered=ImGui::IsItemHovered();
    draw->AddRectFilled(at,end,ImGui::GetColorU32(hovered?ImVec4(0.10F,0.33F,0.50F,1):
        selected?ImVec4(0.08F,0.26F,0.43F,1):ImVec4(0.04F,0.14F,0.27F,1)),10.0F);
    draw->AddRect(at,end,ImGui::GetColorU32(focused||selected?ImVec4(1.0F,0.76F,0.30F,1):
        ImVec4(0.25F,0.43F,0.59F,1)),10.0F,0,focused?3.0F:selected?2.0F:1.0F);
    const auto title_color=ImGui::GetColorU32(ImVec4(1.0F,0.89F,0.62F,1));
    const auto detail_color=ImGui::GetColorU32(ImVec4(0.77F,0.87F,0.91F,1));
    draw->AddText(ImGui::GetFont(),ImGui::GetFontSize()*1.15F,{at.x+18.0F,at.y+15.0F},title_color,
                  choice==77?"US 1.0":"US 1.1");
    const char* badge=!compiled?"NOT IN BUILD":!ready?"ROM REQUIRED":
        playing&&selected?"ACTIVE / NEXT":playing?"ACTIVE":selected?"NEXT START":"SELECT";
    const ImVec2 badge_size=ImGui::CalcTextSize(badge);
    // A narrow panel keeps badge and title on separate lines without adding a
    // second navigation target. The full card remains one gamepad button.
    const bool narrow=version_width<320.0F;
    const ImVec2 badge_at={narrow?at.x+18.0F:end.x-18.0F-badge_size.x,
                          at.y+(narrow?41.0F:18.0F)};
    draw->AddText(badge_at,ImGui::GetColorU32(playing?ImVec4(0.40F,0.96F,0.77F,1):ImVec4(0.83F,0.89F,0.96F,1)),badge);
    const char* detail=choice==77?"Original US release":"Rev A - revised US release";
    draw->AddText({at.x+18.0F,at.y+(narrow?64.0F:53.0F)},detail_color,detail);
    ImGui::EndDisabled();
    if(clicked) {
        if(dkrr_ps5_choose_revision(choice)==0) {
            cached_revisions.preferred=choice;
            revision_choice_status="Selected for the next start. Close DKR-R in the PS5 menu and launch its icon again.";
        } else revision_choice_status="The version could not be selected. The current game and progress are unchanged.";
    }
    ImGui::Dummy({0.0F,10.0F});
}
if(!revision_choice_status.empty())ImGui::TextWrapped("%s",revision_choice_status.c_str());
ImGui::TextWrapped("Controls, picture, sound and texture choices are shared. Adventure progress and Controller Paks stay separate for each version.");
ImGui::Spacing();
ImGui::TextWrapped("Cross selects the next start. Touchpad or Circle returns to the race.");

ImGui::PopTextWrapPos();
