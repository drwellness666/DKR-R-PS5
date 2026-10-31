#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,os,pathlib,subprocess,tempfile
from compiler_flags import imgui_warnings
r=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('game_work',type=pathlib.Path);p.add_argument('host_build',type=pathlib.Path);a=p.parse_args()
s=a.game_work.resolve()/'source/rt64-game/src';imgui=s/'contrib/imgui';plume=s/'contrib/plume';env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
lvp=sorted(pathlib.Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if lvp:env['VK_ICD_FILENAMES']=str(lvp[0])
with tempfile.TemporaryDirectory(prefix='dkrr-frontend-gpu-') as temporary:
    output=pathlib.Path(temporary)/'gpu';flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2','x11'],text=True).split()
    cmd=['clang++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Werror',*imgui_warnings(),'-Wno-unused-private-field','-Wno-missing-field-initializers','-Wno-unused-parameter','-DPLUME_SDL_VULKAN_ENABLED','-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES','-I'+str(a.host_build.resolve()/'generated/enhanced-water/src/contrib/plume'),'-I'+str(r/'src'),'-I'+str(s/'contrib'),'-I'+str(imgui),'-I'+str(plume),'-I'+str(plume/'contrib'),'-I'+str(plume/'contrib/volk'),'-I'+str(plume/'contrib/Vulkan-Headers/include'),'-I'+str(plume/'contrib/VulkanMemoryAllocator/include'),'-I'+str(s/'contrib/volk'),str(r/'tests/ps5_frontend_gpu.cpp'),str(r/'src/ps5_sdl_texture_tracking.cpp'),str(imgui/'backends/imgui_impl_vulkan.cpp'),*[str(imgui/n) for n in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],str(a.host_build.resolve()/'rt64/src/contrib/plume/libplume.a'),*flags,'-ldl','-pthread']
    for name in ('CreateTexture','CreateTextureFromSurface','UpdateTexture','DestroyTexture'):cmd+=['-Wl,--wrap=SDL_'+name]
    subprocess.run(cmd+['-o',str(output)],check=True);subprocess.run([str(output)],check=True,env=env,timeout=60)
