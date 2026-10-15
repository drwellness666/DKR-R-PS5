// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <stdexcept>

namespace dkrr::ps5_profiles {
namespace fs = std::filesystem;

// Preserve the established US1.0 settings in place. Both revisions read and
// write this same configuration; no file migration or merging of save data.
inline fs::path settings(const fs::path& profile,
                         const fs::path& root = "/data/dkrr") {
    if (root.is_absolute() && root == root.lexically_normal() &&
        (profile == root / "profile-us-v77" || profile == root / "profile-us-v80"))
        return root / "profile-us-v77";
    return profile;
}

// Legacy mod preparation receives the shared config, but must create/seed its
// modded save and Pak files in the selected revision's original state root.
inline fs::path state(const fs::path& config, unsigned revision,
                      const fs::path& root = "/data/dkrr") {
    const bool known = root.is_absolute() && root == root.lexically_normal() &&
        (config == root / "profile-us-v77" || config == root / "profile-us-v80");
    if (known && revision != 77 && revision != 80)
        throw std::invalid_argument("The native save revision is not selected.");
    if (known)
        return root / (revision == 80 ? "profile-us-v80" : "profile-us-v77");
    return config;
}
} // namespace dkrr::ps5_profiles
