#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual privately recompiled Options/getter/setter; no console or save writes.
Generated retail code and test executables remain private. Menu graphics, text
loading and hardware EEPROM writes are bounded fixtures, not runtime proof.
"""
import argparse
import importlib.util
import pathlib
import re
import subprocess
import sys
import tempfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-work',type=pathlib.Path,required=True)
parser.add_argument('--pipeline',type=pathlib.Path,required=True)
parser.add_argument('--elf-v77',type=pathlib.Path,required=True)
parser.add_argument('--elf-v80',type=pathlib.Path,required=True)
args=parser.parse_args()
root=pathlib.Path(__file__).resolve().parent.parent
source=args.game_work.resolve()/'source'
sys.path.insert(0,str(source/'DKR-R/scripts'))
from compose_legacy_mod_policy import elf_functions
spec=importlib.util.spec_from_file_location('languages',root/'tools/compose_game_languages.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
for revision,elf in [(77,args.elf_v77),(80,args.elf_v80)]:
    base={'schemaVersion':1}
    policy=module.compose(base,elf,f'us.v{revision}')
    assert base=={'schemaVersion':1} and module.compose(policy,elf,f'us.v{revision}')==policy
    assert len(policy['functionHooks'])==2 and len(policy['instructionPatches'])==2
    conflicting={'instructionPatches':[dict(policy['instructionPatches'][0],value='0x24040002')]}
    try:module.compose(conflicting,elf,f'us.v{revision}')
    except ValueError:pass
    else:raise AssertionError('Conflicting instruction policy was accepted')
    try:module.compose(base,elf,f'us.v{157-revision}')
    except ValueError:pass
    else:raise AssertionError('Wrong revision was accepted')
    symbols=elf_functions(elf,(1,2))
    def address(name):
        choices=symbols.get(name,set())
        assert len(choices)==1,name
        return next(iter(choices))[0]
    folder=args.pipeline.resolve()/f'menu-pipeline/RecompiledFuncs-v{revision}'
    inputs=[folder/(name+'.c') for name in ('menu_options_loop','get_language','set_language')]
    text='\n'.join(path.read_text() for path in inputs)
    assert text.count('dkr_european_language_cycle(rdram, ctx);')==2
    callees=set(re.findall(r'\b([A-Za-z_]\w*)\(rdram, ctx\);',text))
    actual={'menu_options_loop','get_language','set_language','dkr_european_language_cycle'}
    stub_names=callees-actual
    header='#include "recomp.h"\n'+''.join('void '+name+'(uint8_t*,recomp_context*);\n' for name in sorted(callees|actual))
    stubs=''
    for name in sorted(stub_names):
        body={'input_pressed':'ctx->r2=0;',
              'load_menu_text':'++loads; loaded_language=(unsigned)ctx->r4;',
              'mark_write_eeprom_settings':'++writes;'}.get(name,'')
        stubs+='void '+name+'(uint8_t* rdram,recomp_context* ctx){(void)rdram;(void)ctx;'+body+'}\n'
    fixture='''
#include "declarations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned loads,writes,loaded_language;
STUBS
static uint8_t memory[16*1024*1024];
int main(void){
 uint8_t* rdram=memory;
 for(unsigned current=0;current<3;++current) for(int direction=-1;direction<=1;++direction){
  memset(memory,0,sizeof(memory)); recomp_context c={0};c.r29=S32(0x807FF000);c.r4=1;
  const uint64_t preserved=0x1234567802000010ULL;
  SD(preserved|(current*4U),0,S32(EEPROM));
  MEM_W(0,S32(INDEX))=0; MEM_W(0,S32(DELAY))=0;MEM_W(0,S32(IGNORE))=0;
  MEM_B(0,S32(XINPUT))=(int8_t)direction;
  loads=writes=0;loaded_language=99;
  menu_options_loop(rdram,&c);
  const unsigned forward[]={2,0,1},reverse[]={1,2,0};
  unsigned expected=direction>0?forward[current]:direction<0?reverse[current]:current;
  assert(LD(0,S32(EEPROM))==(preserved|(expected*4U)));
  assert(loads==(direction!=0) && writes==(direction!=0));
  if(direction)assert(loaded_language==expected);
  get_language(rdram,&c);assert(c.r2==expected);
  assert(c.r29==S32(0x807FF000));
 }
 for(unsigned current=0;current<3;++current)for(int direction=-1;direction<=1;++direction){
  recomp_context c;memset(&c,0x55,sizeof(c));c.r2=current;c.r7=(uint64_t)(int64_t)direction;
  recomp_context before=c;dkr_european_language_cycle(memory,&c);before.r4=c.r4;
  assert(memcmp(&c,&before,sizeof(c))==0);
 }
 puts("PASS actual US REV Options/getter/setter: both directions, neutral input, original text/save calls, preserved settings/registers");
}
'''.replace('STUBS',stubs).replace('REV',str(revision))
    for token,name in [('EEPROM','sEepromSettings'),('INDEX','gMenuCurIndex'),('DELAY','gMenuDelay'),('IGNORE','gIgnorePlayerInputTime'),('XINPUT','gControllersXAxisDirection')]:
        fixture=fixture.replace(token,hex(address(name)))
    with tempfile.TemporaryDirectory(prefix='dkrr-private-language-') as temp:
        temp=pathlib.Path(temp);(temp/'declarations.h').write_text(header);(temp/'fixture.c').write_text(fixture)
        includes=['-I',str(source/'N64Recomp/include'),'-I',str(root/'src'),'-I',str(temp)]
        objects=[]
        for index,path in enumerate(inputs+[temp/'fixture.c']):
            obj=temp/f'{index}.o';objects.append(str(obj))
            # N64Recomp emits signed LUI shifts intentionally. Exclude this
            # known code-generation idiom only in the generated retail units;
            # the authored adapter/fixture retain every sanitizer check.
            generated_flags=['-fno-sanitize=shift-base'] if path in inputs else []
            subprocess.run(['clang','-std=c11','-O1','-fsanitize=address,undefined',*generated_flags,*includes,'-include',str(temp/'declarations.h'),'-c',str(path),'-o',str(obj)],check=True)
        bridge=temp/'bridge.o';objects.append(str(bridge))
        subprocess.run(['clang++','-std=c++20','-O1','-fsanitize=address,undefined',*includes,'-c',str(root/'src/ps5_game_language.cpp'),'-o',str(bridge)],check=True)
        binary=temp/'test'
        subprocess.run(['clang++','-fsanitize=address,undefined',*objects,'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
