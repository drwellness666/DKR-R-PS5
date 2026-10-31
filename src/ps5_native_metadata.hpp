// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <sys/stat.h>
#include <unistd.h>
namespace dkrr::native_metadata {
// Sandboxed native titles redact uid/nlink to zero. Callers separately enforce
// their exact application root, no-follow paths, file type and permissions.
// This does not authorize a path or substitute for signature/inode checks.
inline bool same_user(const struct stat& st,uid_t owner) {
    if(st.st_uid==owner)return true;
#if defined(__PROSPERO__) || defined(DKRR_TEST_NATIVE_METADATA)
    return owner!=0 && owner==getuid() && st.st_uid==0 && st.st_nlink==0 &&
        st.st_dev!=0 && st.st_ino!=0 && (S_ISREG(st.st_mode)||S_ISDIR(st.st_mode));
#else
    return false;
#endif
}
}
