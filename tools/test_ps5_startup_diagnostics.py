#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the actual native diagnostics reader against ordinary and special files."""
import os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'src/ps5_game_main.cpp').read_text()
reader=s.split('static void configure_render_diagnostics() {',1)[1].split('\nint main()',1)[0]
reader=reader.replace('"/data/dkrr/render-diagnostics.txt"','diagnostics_path')
with tempfile.TemporaryDirectory(prefix='dkrr-startup-reader-') as folder:
 t=pathlib.Path(folder)
 (t/'test.cpp').write_text('''#include "ps5_render_diagnostics.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
static const char* diagnostics_path;
static void report(const char*,...){}
static void configure_render_diagnostics(){'''+reader+'''
int main(int argc,char**argv){if(argc!=3)return 3;diagnostics_path=argv[1];
 for(unsigned i=0;i<DKR_RENDER_DIAGNOSTIC_COUNT;++i)unsetenv(dkrr_render_environment[i]);
 configure_render_diagnostics();unsigned actual=0;
 for(unsigned i=0;i<DKR_RENDER_DIAGNOSTIC_COUNT;++i)if(getenv(dkrr_render_environment[i]))actual|=1U<<i;
 return actual==strtoul(argv[2],nullptr,10)?0:4;
}
''')
 subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(t/'test.cpp'),'-o',str(t/'test')],check=True)
 def run(path,value=0):subprocess.run([str(t/'test'),str(path),str(value)],check=True,timeout=2)
 good=t/'good';good.write_text('track_profile=1\nshader_cache=1\n');run(good,257)
 for data in ('track_profile=1\nunknown=1','track_profile=1\ntrack_profile=0','#'*513,'track_profile=1\0'):
  p=t/'bad';p.write_text(data);run(p);assert p.read_text()==data
 fifo=t/'fifo';os.mkfifo(fifo);run(fifo);assert fifo.is_fifo()
 link=t/'link';link.symlink_to(good);run(link);assert link.is_symlink()
 run(t);run(t/'absent')
print('PASS actual startup diagnostics reader: valid flags; atomic invalid/oversize rejection; preserved FIFO/symlink/directory and missing file without blocking')
