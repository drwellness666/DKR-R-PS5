// SPDX-License-Identifier: GPL-3.0-or-later
// Actual storage worker, disposable PS5-shaped profiles and simulated redacted
// link counts. No console paths or owner data are opened.
#include "storage/maintenance.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
static void put(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << bytes;
    assert(out.good());
}
static std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.good());
    return {std::istreambuf_iterator<char>(input), {}};
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const fs::path root = fs::absolute(argv[1]);
    assert(!fs::exists(root));
    const auto shared = root / "data/dkrr/profile-us-v77";
    const auto other = root / "data/dkrr/profile-us-v80";
    const auto app = root / "data/homebrew/PPSA94678";
    const auto prefs = root / "data/dkrr/preferences/update-client-0";
    const std::vector<fs::path> protected_files{
        shared / "saves/dkr.us.v77.bin", other / "saves/dkr.us.v77.bin",
        shared / "dkr-port-settings.ini", shared / "mod-store/account.json",
        shared / "mods/legacy/user-content.bin", shared / "texture-packs/user.rtz",
        root / "data/dkrr/roms/owned-fixture.z64", prefs / "native-install/ready",
        prefs / "native-install/old/eboot.bin", app / "eboot.bin"};
    for (const auto& path : protected_files) put(path, path.generic_string());
    // No large/proprietary data: all fixtures are short plain text.
    const auto image = shared / "mod-store/images" / std::string(64, 'a');
    const auto linked = shared / "mod-store/images" / std::string(64, 'b');
    const auto bad = shared / "mod-store/images" / std::string(64, 'c');
    const auto symlink = shared / "mod-store/images" / std::string(64, 'd');
    put(image, "discard this old cache");
    put(linked, "keep this hard-linked cache");
    put(bad, "keep on metadata failure");
    fs::create_hard_link(linked, root / "hard-link-owner");
    fs::create_symlink(protected_files.front(), symlink);
    fs::last_write_time(image, fs::file_time_type::clock::now() - std::chrono::hours(24 * 40));
    fs::last_write_time(linked, fs::file_time_type::clock::now() - std::chrono::hours(24 * 40));
    fs::last_write_time(bad, fs::file_time_type::clock::now() - std::chrono::hours(24 * 40));
    // A prior preference file must remain readable with the native zero count.
    put(shared / "storage/preferences.json", R"({"schema":1,"automatic":true})");
    dkr::storage::start(shared);
    assert(dkr::storage::automatic());
    // The actual startup worker starts one automatic pass after ten seconds.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);
    while (fs::exists(image) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    dkr::storage::shutdown();
    assert(!fs::exists(image));
    assert(fs::exists(linked) && fs::exists(bad) && fs::is_symlink(symlink));
    for (const auto& path : protected_files) assert(read(path) == path.generic_string());
    // Reload an explicit opt-out; changing it is durable and never a default reset.
    put(shared / "storage/preferences.json", R"({"schema":1,"automatic":false})");
    dkr::storage::start(shared);
    assert(!dkr::storage::automatic());
    dkr::storage::automatic(true);
    dkr::storage::shutdown();
    assert(read(shared / "storage/preferences.json").find("true") != std::string::npos);
    // A pass that never finished (marker left behind) is skipped once, then runs again.
    const auto later = shared / "mod-store/images" / std::string(64, 'f');
    put(later, "discard after the skipped launch");
    fs::last_write_time(later, fs::file_time_type::clock::now() - std::chrono::hours(24 * 40));
    put(shared / "storage/automatic-pass.running", "");
    put(shared / "storage/preferences.json", R"({"schema":1,"automatic":true})");
    dkr::storage::start(shared);
    std::this_thread::sleep_for(std::chrono::seconds(13));
    dkr::storage::shutdown();
    assert(fs::exists(later) && !fs::exists(shared / "storage/automatic-pass.running"));
    dkr::storage::start(shared);
    const auto again = std::chrono::steady_clock::now() + std::chrono::seconds(25);
    while (fs::exists(later) && std::chrono::steady_clock::now() < again)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    dkr::storage::shutdown();
    assert(!fs::exists(later) && !fs::exists(shared / "storage/automatic-pass.running"));
    // A second manual clean must find nothing new: no recreated "disposable" files and no growing counters.
    {
        const auto old_image = shared / "mod-store/images" / std::string(64, '9');
        put(old_image, "old cache");
        fs::last_write_time(old_image, fs::file_time_type::clock::now() - std::chrono::hours(24 * 40));
        put(shared / "storage/preferences.json", R"({"schema":1,"automatic":false})");
        dkr::storage::start(shared);
        auto settle = [&](std::chrono::seconds limit) {
            const auto end = std::chrono::steady_clock::now() + limit;
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            while (dkr::storage::view().busy && std::chrono::steady_clock::now() < end)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
        };
        dkr::storage::request(dkr::storage::Action::Clean);
        settle(std::chrono::seconds(20));
        assert(!fs::exists(old_image));
        const auto first = dkr::storage::view();
        dkr::storage::request(dkr::storage::Action::Clean);
        settle(std::chrono::seconds(20));
        const auto second = dkr::storage::view();
        std::cout << "clean 1: files=" << first.removed_files << " bytes=" << first.removed_bytes
                  << "; clean 2: files=" << second.removed_files << " bytes=" << second.removed_bytes << "\n";
        assert(second.removed_files == first.removed_files && second.removed_bytes == first.removed_bytes);
        dkr::storage::shutdown();
    }
    std::cout << "PASS actual PS5 storage worker: automatic startup with zero link counts, persisted opt-out, "
                 "hard links/errors retained, both saves/ROM/settings/account/mods/textures/native update recovery preserved\n";
}
