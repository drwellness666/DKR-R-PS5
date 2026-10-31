// SPDX-License-Identifier: GPL-3.0-or-later
#include "save_manager.hpp"
#include "ps5_store_lease.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
static std::vector<std::string> messages,buttons;
static std::string bad_lease_parent;
static unsigned lease_parent_checks=0;
extern "C" int __wrap_lstat(const char* path,struct stat* info){
 ++lease_parent_checks;*info={};info->st_mode=std::string(path)==bad_lease_parent?S_IFLNK|0777:S_IFDIR|0700;return 0;
}
static float panel_width=800;
static void text(std::string_view s){messages.emplace_back(s);ImGui::TextUnformatted(s.data(),s.data()+s.size());}
static void gap(float n){ImGui::Dummy({0,n});}
static bool has(const char*s){return std::any_of(messages.begin(),messages.end(),[&](const auto&v){return v.find(s)!=std::string::npos;});}
namespace saves {
using dkr::runtime::saves::codec::SaveImage;
struct View{bool valid=false;dkr::runtime::saves::SaveInfo adventure;std::optional<SaveImage> image;};static View current;
static bool busy=false,lobby=false;static unsigned backups=0,exports=0;static std::function<bool(std::string&)> pending;
const View&CachedSaveManagerView(){return current;}
void SavesNote(std::string_view s,float,float=16,unsigned=0){text(s);}void SavesSeparator(std::string_view s,float){text(s);}void PaddockGap(float n){gap(n);}
constexpr unsigned kSetText=0xFFFFFF;enum class SavesTone{Primary,Plain};
bool SharedSaveBusy(){return busy;}
bool SavesRaceButton(const char*s,float,SavesTone,bool disabled){buttons.emplace_back(s);ImGui::BeginDisabled(disabled);bool pressed=ImGui::Button(s,{panel_width,44});ImGui::EndDisabled();return pressed&&!disabled;}
bool BeginSharedSaveJob(std::function<bool(std::string&)> action,const char*){assert(!busy);busy=true;pending=std::move(action);return true;}
bool ExportAdventureWithDialog(){++exports;return true;}
#include "live_saves.inc"
void draw(){DrawSavesLivePage(panel_width,lobby);}
}
namespace dkr::runtime::saves {bool backup_adventure(std::filesystem::path&,std::string&){++::saves::backups;return true;}}
namespace dkr::store {
struct InstallView{bool ready=false,busy=false;std::string stage,error;};
struct Fixture{std::shared_ptr<InstallView> state=std::make_shared<InstallView>();unsigned reads=0,retries=0;
 auto view(){++reads;return state;}void retry_recovery(){++retries;}};
Fixture&installations(){static Fixture value;return value;}
}
namespace dkr::runtime::netplay {static bool active=false;struct Session{bool active(){return netplay::active;}};Session&session(){static Session value;return value;}}
namespace online {
void DrawOlNote(std::string_view s,float){text(s);}void PaddockGap(float n){gap(n);}enum class OlCtaColour{Blue};
bool OlCta(const char*s,OlCtaColour,float){buttons.emplace_back(s);return ImGui::Button(s,{panel_width,44});}
#include "store_status.inc"
static bool launcher=false,admitted=false;
void draw(){admitted=DrawOnlineStoreStatus(panel_width,launcher);}
}
static void(*draw_page)()=saves::draw;
static void frame(bool focus=false){
 messages.clear();buttons.clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1100,650});ImGui::Begin("Menu",nullptr,ImGuiWindowFlags_NoTitleBar);
 if(focus)ImGui::SetKeyboardFocusHere();draw_page();ImGui::End();ImGui::Render();
}
static void press(ImGuiKey key){auto&io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();frame();io.AddKeyEvent(key,false);frame();frame();}
static void new_page(void(*fn)()){ImGui::GetCurrentContext()->NavId=0;draw_page=fn;frame(true);frame();press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadDpadUp);}
static void check_lease(){
 for(const auto*root:{"/data/dkrr/profile-us-v77","/data/dkrr/profile-us-v80"}){
  const auto path=std::filesystem::path(root)/"mod-store/install.lock";
  bad_lease_parent.clear();assert(dkrr::ps5_store::own_lease_path(path));
  for(const auto&parent:{std::filesystem::path("/data"),std::filesystem::path("/data/dkrr"),std::filesystem::path(root),std::filesystem::path(root)/"mod-store"}){
   bad_lease_parent=parent;assert(!dkrr::ps5_store::own_lease_path(path));
  }
 }
 bad_lease_parent.clear();lease_parent_checks=0;
 for(const auto*bad:{"/tmp/install.lock","/data/dkrr/profile-us-v77/mod-store/../install.lock","/data/dkrr/profile-us-v77/mod-store/content.zip"})assert(!dkrr::ps5_store::own_lease_path(bad));
 assert(lease_parent_checks==0);
 struct stat file{},entry{};file.st_mode=S_IFREG|0600;file.st_uid=1;file.st_dev=3;file.st_ino=12;entry=file;
 const auto path="/data/dkrr/profile-us-v77/mod-store/install.lock";
 assert(dkrr::ps5_store::own_lease_metadata(path,file,entry,1));
 file.st_uid=entry.st_uid=0;assert(dkrr::ps5_store::own_lease_metadata(path,file,entry,1));
 entry.st_uid=1;assert(!dkrr::ps5_store::own_lease_metadata(path,file,entry,1));
 file.st_uid=entry.st_uid=1;
 assert(dkrr::ps5_store::own_lease_metadata("/data/dkrr/profile-us-v80/mod-store/install.lock",file,entry,1));
 for(auto bad:{"/tmp/install.lock","/data/dkrr/profile-us-v77/mod-store/content.zip","/data/dkrr/profile-us-v77/mod-store/../install.lock"})assert(!dkrr::ps5_store::own_lease_metadata(bad,file,entry,1));
 for(unsigned n=0;n<7;++n){auto changed=entry;if(n==0)changed.st_nlink=2;if(n==1)changed.st_mode=S_IFLNK|0777;if(n==2)changed.st_uid=2;if(n==3)changed.st_ino=13;if(n==4)changed.st_dev=4;if(n==5)changed.st_size=1;if(n==6)changed.st_mode=S_IFDIR|0700;assert(!dkrr::ps5_store::own_lease_metadata(path,file,changed,1));}
 file.st_nlink=2;assert(!dkrr::ps5_store::own_lease_metadata(path,file,entry,1));
}
int main(){
 check_lease();
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.DisplaySize={1200,900};io.DeltaTime=1.0F/60;io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;unsigned char*p;int x,y;io.Fonts->GetTexDataAsRGBA32(&p,&x,&y);
 draw_page=saves::draw;frame();assert(has("Reading saved progress")&&buttons.empty());
 saves::current.valid=true;frame();assert(has("No Adventure progress")&&buttons.empty());
 saves::current.adventure={"/test/personal.bin",true,true,512};saves::current.image.emplace();saves::current.image->slots[0].name="DKR";saves::current.image->slots[0].balloons[0]=7;
 new_page(saves::draw);assert(has("DKR - 7 / 47")&&has("disabled during play")&&buttons==std::vector<std::string>({"BACK UP SAVE","EXPORT SAVE"}));
 press(ImGuiKey_GamepadFaceDown);assert(saves::busy&&saves::pending&&saves::backups==0);press(ImGuiKey_GamepadFaceDown);assert(saves::backups==0);
 std::string error;assert(saves::pending(error));saves::busy=false;assert(saves::backups==1);new_page(saves::draw);press(ImGuiKey_GamepadDpadDown);press(ImGuiKey_GamepadFaceDown);assert(saves::exports==1);
 saves::lobby=true;frame();assert(has("Online progress is separate"));
 draw_page=online::draw;auto&store=dkr::store::installations();store.state->error="The mod operation lock is unsafe.";frame();assert(!online::admitted&&has("lock is unsafe")&&buttons.empty());
 online::launcher=true;new_page(online::draw);press(ImGuiKey_GamepadFaceDown);assert(store.retries==1);
 store.state->busy=true;store.state->stage="Checking mod installations";frame();assert(!online::admitted&&has("Checking mod installations")&&!has("lock is unsafe")&&buttons.empty());
 store.state->busy=false;store.state->ready=true;frame();assert(online::admitted&&messages.empty());
 store.state->ready=false;dkr::runtime::netplay::active=true;frame();assert(messages.empty()&&buttons.empty());
 ImGui::DestroyContext();puts("PASS actual menu functions: committed-slot preview and guarded nonblocking backup/export, exact Store readiness/error/retry/active-session handling, native lease scope and alias/owner/substitution rejection");
}
