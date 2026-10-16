#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Shared host check and package plan; never executes a package manager."""
import json, pathlib, platform, shutil, subprocess, sys
TOOLS=('bash','make','git','curl','wget','tar','unzip','patch','pkg-config','clang','clang++','llvm-config','glslangValidator','flock','cmake','ld.lld','spirv-as','llvm-ar','llvm-ranlib','g++','bison','flex')
PACKAGES={
 'apt':('build-essential','git','python3','python3-venv','cmake','clang','libclang-dev','llvm-dev','lld','curl','wget','unzip','patch','pkg-config','libpcre2-dev','glslang-tools','imagemagick','spirv-tools','spirv-headers','bison','flex','util-linux'),
 'pacman':('base-devel','git','python','cmake','clang','llvm','lld','curl','wget','unzip','patch','pkgconf','pcre2','glslang','imagemagick','spirv-tools','spirv-headers','bison','flex'),
 'dnf':('gcc','gcc-c++','make','git','python3','python3-pip','cmake','clang','clang-devel','llvm-devel','lld','curl','wget','unzip','patch','pkgconf-pkg-config','pcre2-devel','glslang','ImageMagick','spirv-tools','spirv-headers','bison','flex','util-linux'),
}
def package_plan(info=None):
    try: info=info or platform.freedesktop_os_release()
    except OSError: info={}
    family={info.get('ID',''),*info.get('ID_LIKE','').split()}
    if family & {'ubuntu','debian'}: manager='apt'
    elif family & {'arch','cachyos','manjaro'}: manager='pacman'
    elif family & {'fedora','rhel'}: manager='dnf'
    else: return []
    return {'apt':['apt-get','install','--no-install-recommends','-y'], 'pacman':['pacman','-S','--needed','--noconfirm'], 'dnf':['dnf','install','-y']}[manager]+list(PACKAGES[manager])
def inspect(root,which=shutil.which):
    missing=[x for x in TOOLS if not which(x)]
    if not (which('magick') or which('convert')): missing.append('ImageMagick')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'): missing.append('Linux x86-64 (native or WSL2)')
    if sys.version_info<(3,12): missing.append('Python 3.12 or newer')
    pcre=bool(which('pkg-config')) and subprocess.run(['pkg-config','--exists','libpcre2-8'],capture_output=True).returncode==0
    if not pcre: missing.append('PCRE2 development files')
    plan=package_plan()
    return {'missing':missing,'package_argv':plan,'package_command':'sudo '+' '.join(plan) if plan else '', 'free_gib':round(shutil.disk_usage(root).free/2**30,1),'ok':not missing,'note':'The first build needs several GB and internet access for pinned public sources. ROMs stay local.'}
if __name__=='__main__':
    result=inspect(pathlib.Path(__file__).resolve().parents[1]);print(json.dumps(result) if '--json' in sys.argv else '\n'.join(result['missing']) or 'Build prerequisites ready')
    sys.exit(0 if result['ok'] else 1)
