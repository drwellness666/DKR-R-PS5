// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace dkrr::ps5_textures {
namespace fs = std::filesystem;

// The shared configuration retains enabled/hidden/priority choices. Existing
// archives in the sibling folder remain usable without copying or migrating.
inline fs::path sibling_library(const fs::path& active,
                               const fs::path& root = "/data/dkrr") {
    if (!root.is_absolute() || root != root.lexically_normal() ||
        active != active.lexically_normal()) return {};
    if (active == root / "profile-us-v77/texture-packs")
        return root / "profile-us-v80/texture-packs";
    if (active == root / "profile-us-v80/texture-packs")
        return root / "profile-us-v77/texture-packs";
    return {};
}

inline bool shared(const fs::path& file, const fs::path& active,
                   const fs::path& root = "/data/dkrr") {
    const auto sibling = sibling_library(active, root);
    return !sibling.empty() && file == file.lexically_normal() &&
           file.parent_path() == sibling;
}

inline bool ordinary_tree(const fs::path& path) {
    if (!path.is_absolute()) return false;
    fs::path prefix;
    std::error_code error;
    for (const auto& component : path) {
        prefix /= component;
        if (!fs::is_directory(fs::symlink_status(prefix, error)) || error)
            return false;
    }
    return true;
}

inline std::string identity(const fs::path& file) {
    std::string name = file.filename().string();
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

inline std::vector<fs::directory_entry> entries(
        const fs::path& active, std::error_code& error,
        const fs::path& root = "/data/dkrr") {
    error.clear();
    std::vector<fs::directory_entry> result;
    std::set<std::string> seen;
    auto append = [&](const fs::path& library, std::error_code& status) {
        std::vector<fs::directory_entry> candidates;
        fs::directory_iterator at(library, status), end;
        while (!status && at != end) {
            std::error_code inspection;
            const auto kind = at->symlink_status(inspection);
            if (!inspection && !fs::is_symlink(kind) &&
                (fs::is_regular_file(kind) || fs::is_directory(kind))) {
                const auto extension = identity(at->path().extension());
                if ((fs::is_directory(kind) && extension != ".importing") ||
                    extension == ".zip" || extension == ".rtz")
                    candidates.push_back(*at);
            }
            at.increment(status);
        }
        // An inaccessible/partly enumerated sibling never supplies partial cards.
        if (status) return;
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.path().filename().native() < b.path().filename().native();
        });
        for (auto& entry : candidates)
            if (seen.insert(identity(entry.path())).second) result.push_back(std::move(entry));
    };
    append(active, error);
    if (error) return {};
    const auto sibling = sibling_library(active, root);
    if (!sibling.empty() && ordinary_tree(sibling)) {
        std::error_code optional_error;
        append(sibling, optional_error);
    }
    return result;
}
} // namespace dkrr::ps5_textures
