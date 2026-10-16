// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include "ps5_save_metadata_protocol.h"
namespace dkrr::ps5_saves {
// A native stat link count of zero is unavailable metadata, never a fabricated
// claim that the inode has one link. Admit it only for operations that cannot
// overwrite an existing inode's contents: atomic entry replacement, read-only
// access under a separate lease, or removal of our history entry. Global stat
// results, foreign paths and generic callers retain their original semantics.
inline bool transaction_path(const std::filesystem::path& path) {
    const auto text=path.generic_string();
    return text.size()<DKR_SAVE_METADATA_PATH_MAX && dkrr_metadata_allowed(text.c_str());
}
}
