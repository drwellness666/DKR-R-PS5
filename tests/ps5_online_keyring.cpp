// SPDX-License-Identifier: GPL-3.0-or-later
// "Keep me signed in" must survive a restart on the PS5.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "ps5_online_keyring.hpp"
namespace fs = std::filesystem;
namespace keyring = dkr::online::keyring;
int main() {
    char pattern[] = "/tmp/dkrr-keyring-XXXXXX";
    const std::string root = mkdtemp(pattern);
    const std::string token(43, 'a');
    assert(keyring::available());
    assert(keyring::load(root).empty());                      // Nothing saved yet.
    assert(keyring::store(root, token));
    assert(keyring::load(root) == token);                     // Survives a "restart".
    struct stat info{};
    assert(stat((root + "/device-session/device.token").c_str(), &info) == 0 && (info.st_mode & 0777) == 0600);
    assert(stat((root + "/device-session").c_str(), &info) == 0 && (info.st_mode & 0777) == 0700);
    const std::string second = std::string(42, 'b') + "-";
    assert(keyring::store(root, second) && keyring::load(root) == second); // Replace in place.
    assert(!keyring::store(root, "short") && keyring::load(root) == second); // Invalid input keeps the old token.
    assert(!keyring::store(root, std::string(43, '/')));
    chmod((root + "/device-session/device.token").c_str(), 0644); // Group/world readable is refused.
    assert(keyring::load(root).empty());
    chmod((root + "/device-session/device.token").c_str(), 0600);
    assert(keyring::load(root) == second);
    { std::ofstream bad(root + "/device-session/device.token", std::ios::trunc); bad << "tampered"; }
    assert(keyring::load(root).empty());                      // Wrong length refused.
    assert(keyring::store(root, token));
    keyring::clear(root);
    assert(keyring::load(root).empty());                      // Sign-out removes it.
    assert(!keyring::store("relative/path", token));          // Only absolute profile roots.
    fs::remove_all(root);
    std::puts("PASS PS5 keep-me-signed-in token store");
}
