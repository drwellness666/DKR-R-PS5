# SPDX-License-Identifier: GPL-3.0-or-later
"""Feature-detect only the known third-party ImGui diagnostic suppressions."""
import subprocess
def imgui_warnings():
    result=[]
    for flag in ('-Wno-nontrivial-memcall','-Wno-nontrivial-memaccess','-Wno-uninitialized-const-pointer','-Wno-uninitialized-const-reference'):
        probe=subprocess.run(['clang++','-x','c++','-fsyntax-only','-Werror',flag,'-'],input='',text=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        if probe.returncode==0:result.append(flag)
    return result
