// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>
static std::mutex g_platform_mutex;
static std::filesystem::path g_user_mapping_path;
namespace dkr::runtime::platform {
bool export_controller_mappings(const std::filesystem::path&, std::string&);
}
#include "file_exports.inc"
static void put(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary); file << text; assert(file.good());
}
static std::string get(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
int main(int argc,char**argv) {
    assert(argc==2); namespace fs=std::filesystem;
    const fs::path root=argv[1]; assert(fs::create_directory(root));
    g_user_mapping_path=root/"own-mappings.txt";
    const std::string mappings="own controller mappings\n";
    put(g_user_mapping_path,mappings); std::string status;
    const auto export_file=root/"export.txt";
    assert(dkr::runtime::platform::export_controller_mappings(export_file,status));
    assert(get(export_file)==mappings&&get(g_user_mapping_path)==mappings);
    put(export_file,"KEEP existing destination");
    assert(!dkr::runtime::platform::export_controller_mappings(export_file,status));
    assert(get(export_file)=="KEEP existing destination"&&!status.empty());
    const auto victim=root/"victim.txt";put(victim,"KEEP link target");
    const auto link=root/"link.txt";fs::create_symlink(victim,link);
    assert(!dkr::runtime::platform::export_controller_mappings(link,status));
    assert(fs::is_symlink(link)&&get(victim)=="KEEP link target");
    const auto absent=root/"absent.txt",dangling=root/"dangling.txt";
    fs::create_symlink(absent,dangling);
    assert(!dkr::runtime::platform::export_controller_mappings(dangling,status));
    assert(fs::is_symlink(dangling)&&!fs::exists(absent));
    const auto hardlink=root/"hardlink.txt";fs::create_hard_link(victim,hardlink);
    assert(!dkr::runtime::platform::export_controller_mappings(hardlink,status));
    assert(get(victim)=="KEEP link target"&&fs::equivalent(victim,hardlink));
    const auto directory=root/"directory";assert(fs::create_directory(directory));
    assert(!dkr::runtime::platform::export_controller_mappings(directory,status));
    assert(fs::is_empty(directory)&&get(g_user_mapping_path)==mappings);
    g_user_mapping_path=root/"missing-source.txt";
    assert(!dkr::runtime::platform::export_controller_mappings(root/"not-created.txt",status));
    assert(!fs::exists(root/"not-created.txt"));
    std::puts("PASS actual PS5 mapping export: fresh file, existing-file refusal, symlink/dangling/hardlink preservation, directory and missing-source refusal");
}
