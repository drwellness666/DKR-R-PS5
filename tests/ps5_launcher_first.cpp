// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the production startup argv, initial revision routing, launcher
// update handoff and return confirmation with only external owners stubbed.
#include "ps5_profile_paths.hpp"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace dkr::runtime {
namespace rom { enum class Revision { UsV77, UsV80 }; struct Identity { Revision revision; }; }
namespace ui { enum class LifecycleRequest { None, StopGame, Exit }; }
namespace pak { inline fs::path configured; void configure(const fs::path& p) { configured=p; } }
namespace saves { inline fs::path configured; void configure(const fs::path& p) { configured=p; } }
}
static unsigned active=77, directories=0;
unsigned dkrr_ps5_active_revision() { return active; }
void dkrr_ps5_set_active_revision(unsigned revision) { active=revision; }
void fixture_create_directory(const fs::path&) { ++directories; }
static void route(dkr::runtime::rom::Revision revision) {
    const dkr::runtime::rom::Identity rom_identity{revision};
    const fs::path config_directory="/data/dkrr/profile-us-v77";
    fs::path profile_directory=active==80?"/data/dkrr/profile-us-v80":"/data/dkrr/profile-us-v77";
    #include "launcher_profile.inc"
    assert(profile_directory==(active==80?"/data/dkrr/profile-us-v80":"/data/dkrr/profile-us-v77"));
}
int dkrr_game_main(int argc, char** argv) {
    assert(argc==3 && argv[3]==nullptr);
    assert(std::string(argv[0])=="/app0/eboot.bin" && std::string(argv[1])=="--config");
    assert(std::string(argv[2])=="/data/dkrr/profile-us-v80");
    for(int i=0;i<argc;++i)assert(std::string(argv[i])!="--rom");
    return 0;
}
namespace dkrr::updates { inline bool ready=false; bool close_requested() { return ready; } }
static void update_handoff(bool ready) {
    using LifecycleRequest=dkr::runtime::ui::LifecycleRequest;
    struct Result { bool start_game=true; LifecycleRequest lifecycle_request=LifecycleRequest::None; } result;
    dkrr::updates::ready=ready;
    unsigned ticks=0;
    while(true) {
        #include "launcher_update.inc"
        ++ticks;
        break;
    }
    assert(ticks==(ready?0U:1U));
    assert(result.start_game==!ready);
    assert(result.lifecycle_request==(ready?LifecycleRequest::Exit:LifecycleRequest::None));
}

// Modal test doubles: a return request is forbidden without confirmation.
static bool clicked=false, confirm=false, popup=false;
static std::atomic<int> g_native_frontend_page{5};
static std::atomic<bool> g_native_online_lobby_requested{false}, g_overlay_visible{true};
static std::atomic<dkr::runtime::ui::LifecycleRequest> g_lifecycle_request{dkr::runtime::ui::LifecycleRequest::None};
constexpr int kPagePlay=0;
namespace ImGui { void OpenPopup(const char*) { popup=true; } void CloseCurrentPopup() { popup=false; } void SameLine() {} }
void PaddockGap(float) {}
bool PaddockImportButton(const char*,float) { return clicked; }
bool BeginSettingsDialog(const char*,const char*,float,unsigned) { return popup; }
void EndSettingsDialog() {}
int PaddockReading(float,bool,float) { return 0; }
unsigned PaddockRgb(unsigned v) { return v; }
float SettingsDialogInner() { return 600; }
void PaddockText(int,unsigned,const char*,float) {}
bool PlayNoticeButton(const char* label) { return confirm&&std::string(label)=="RETURN TO LAUNCHER"; }
static void return_dialog(bool live) {
    const float width=800;
    #include "launcher_return.inc"
}
int main() {
    const char* profile="/data/dkrr/profile-us-v80";
    #include "launcher_argv.inc"
    assert(dkrr_game_main(3,args)==0);
    for(unsigned preferred:{77U,80U})for(unsigned selected:{77U,80U}) {
        active=preferred; directories=0;
        dkr::runtime::pak::configured.clear();dkr::runtime::saves::configured.clear();
        route(selected==80?dkr::runtime::rom::Revision::UsV80:dkr::runtime::rom::Revision::UsV77);
        assert(active==selected && directories==(selected!=preferred?1U:0U));
        if(selected!=preferred) {
            const fs::path expected=selected==80?"/data/dkrr/profile-us-v80":"/data/dkrr/profile-us-v77";
            assert(dkr::runtime::pak::configured==expected && dkr::runtime::saves::configured==expected);
        }
    }
    update_handoff(false); update_handoff(true);
    clicked=true;return_dialog(false);assert(!popup);
    return_dialog(true);assert(popup && g_lifecycle_request==dkr::runtime::ui::LifecycleRequest::None);
    confirm=true;clicked=false;return_dialog(true);
    assert(!popup && !g_overlay_visible && g_native_online_lobby_requested && g_native_frontend_page==kPagePlay);
    assert(g_lifecycle_request==dkr::runtime::ui::LifecycleRequest::StopGame);
    puts("PASS production launcher-first fragments: no ROM autostart argv, all four revision/save routes, updater exit and confirmed return request");
}
