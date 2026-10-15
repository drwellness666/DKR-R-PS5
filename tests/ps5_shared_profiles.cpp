// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_profile_paths.hpp"
#include "ps5_texture_library.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <chrono>
namespace fs=std::filesystem;
std::string read(const fs::path&p){std::ifstream s(p,std::ios::binary);return {std::istreambuf_iterator<char>(s),{}};}
void write(const fs::path&p,const std::string&v){fs::create_directories(p.parent_path());std::ofstream s(p,std::ios::binary);s<<v;assert(bool(s));}
int main(int argc,char**argv){
 assert(argc==2);const fs::path root=argv[1];using dkrr::ps5_profiles::settings;using dkrr::ps5_profiles::state;
 const auto us10=root/"profile-us-v77",us11=root/"profile-us-v80";
 assert(settings(us10,root)==us10&&settings(us11,root)==us10);
 for(const auto&name:{"dkr-port-settings.ini","dkr-port-ui.ini","hud-layouts-v2.json","texture-packs.ini","controllers/gamecontrollerdb.txt"}){
  write(us10/name,std::string("US10 original ")+name);write(us11/name,std::string("US11 old preserved ")+name);
  const auto old=read(us11/name);const auto stamp=fs::last_write_time(us11/name);
  assert(read(settings(us11,root)/name)==read(settings(us10,root)/name));
  write(settings(us11,root)/name,std::string("changed from US11 ")+name);
  assert(read(settings(us10,root)/name)==std::string("changed from US11 ")+name);
  assert(read(us11/name)==old&&fs::last_write_time(us11/name)==stamp);
 }
 for(unsigned revision:{77U,80U})assert(state(settings(us11,root),revision,root)==(revision==77?us10:us11));
 for(const auto& name:{"saves/dkr.us.v77.bin","controller-pak-1.mpk","save-backups/own.bin","save-history/own.bin","saves/mods/exact-fingerprint/paks/controller-pak-1.mpk"}){
  write(us10/name,"US10 progress");write(us11/name,"US11 progress");
  const auto stamp=fs::last_write_time(us10/name);
  write(state(settings(us11,root),80,root)/name,"US11 new progress");
  assert(read(us10/name)=="US10 progress"&&fs::last_write_time(us10/name)==stamp);
 }
 const auto unknown=root/"custom-profile";assert(settings(unknown,root)==unknown&&state(unknown,80,root)==unknown);
 assert(settings(root/"profile-us-v80/../profile-us-v80",root)!=us10);
 bool refused=false;try{(void)state(us10,0,root);}catch(const std::invalid_argument&){refused=true;}assert(refused);
 auto a=us10/"texture-packs",b=us11/"texture-packs";fs::create_directories(a);fs::create_directories(b);
 write(a/"shared.zip","primary pack");write(b/"shared.zip","different legacy pack");write(b/"only-us11.rtz","legacy texture archive");
 write(b/"unfinished.importing/file.png","partial");write(b/"notes.txt","ignored");write(b/"converted/rt64.json","{}");
 fs::create_symlink(b/"only-us11.rtz",a/"link.zip");fs::create_directory_symlink(b/"converted",a/"linked-folder");
 std::error_code error;const auto stamp=fs::last_write_time(b/"shared.zip");
 auto entries=dkrr::ps5_textures::entries(a,error,root);assert(!error&&entries.size()==3);
 assert(entries[0].path()==a/"shared.zip");
 bool found=false;for(const auto&e:entries){if(e.path().filename()=="only-us11.rtz"){found=true;assert(dkrr::ps5_textures::shared(e.path(),a,root));}}
 assert(found&&read(b/"shared.zip")=="different legacy pack"&&fs::last_write_time(b/"shared.zip")==stamp);
 assert(!dkrr::ps5_textures::shared(a/"shared.zip",a,root));
 auto saved=us11/"texture-packs-original";fs::rename(b,saved);fs::create_directory_symlink(saved,b);
 entries=dkrr::ps5_textures::entries(a,error,root);assert(!error&&entries.size()==1);fs::remove(b);
 entries=dkrr::ps5_textures::entries(a,error,root);assert(!error&&entries.size()==1);
 auto missing=dkrr::ps5_textures::entries(root/"missing",error,root);assert(error&&missing.empty());
 std::cout<<"PASS shared settings read/write in both revisions; original settings retained; EEPROM/Pak/backups/mod-state isolated; existing texture union, precedence, no copies, symlink/partial/absent refusal\n";
}
