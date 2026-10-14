// SPDX-License-Identifier: GPL-3.0-or-later
// Called only inside the native pause page; filesystem names are not renamed.
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
ImGui::TextUnformatted("GAME VERSION");
ImGui::Text("Playing: Diddy Kong Racing - US %s",revisions.active==80?"1.1 (Rev A)":"1.0");
ImGui::Text("Next start: US %s",revisions.preferred==80?"1.1":"1.0");
ImGui::TextWrapped("Choose the original US 1.0 or revised US 1.1. Each version keeps its own progress and settings.");
for(unsigned choice:{77U,80U}) {
    const unsigned flag=choice==77?1U:2U;
    const bool ready=(revisions.compiled_mask&flag)&&(revisions.available_mask&flag);
    ImGui::BeginDisabled(!ready);
    if(ImGui::RadioButton(choice==77?"US 1.0 - original":"US 1.1 - Rev A",revisions.preferred==choice)) {
        if(dkrr_ps5_choose_revision(choice)==0) {
            cached_revisions.preferred=choice;
            revision_choice_status="Version selected for the next start. Use Restart DKR-R, then launch the title again.";
        } else revision_choice_status="The version could not be selected. Your current game and saves have not changed.";
    }
    ImGui::EndDisabled();
    if(!(revisions.compiled_mask&flag))ImGui::TextUnformatted("This version is not included in this build.");
    else if(!(revisions.available_mask&flag))ImGui::TextUnformatted("Add your own matching ROM to enable this version.");
}
if(!revision_choice_status.empty())ImGui::TextWrapped("%s",revision_choice_status.c_str());
ImGui::Spacing();
ImGui::TextWrapped("L1 / R1 opens settings pages. Touchpad or Circle returns to the race.");
