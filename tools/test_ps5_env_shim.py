#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Each host-side PS5 rule must change exactly the behaviour it claims, and nothing else."""
import os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
EXPECT={ # rule -> (key, value without the rule, value with the rule)
 'proc':('proc','1','0'),'nlink0':('nlink','1','0'),'uid0':('uid_is_owner','1','0'),
 'flock':('flock','1','0'),'chmod':('chmod','1','0'),'fsync':('fsync','1','0')}
with tempfile.TemporaryDirectory(prefix='dkrr-env-shim-') as temp:
    work=pathlib.Path(temp)
    subprocess.run(['clang','-std=c11','-O1','-Wall','-Wextra','-Werror','-c',str(root/'tests/ps5_env_shim.c'),'-o',str(work/'shim.o')],check=True)
    subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror',str(root/'tests/ps5_env_probe.cpp'),str(work/'shim.o'),'-ldl','-o',str(work/'probe')],check=True)
    def run(rules):
        folder=work/('data-'+(rules or 'none').replace(',','-'));folder.mkdir(exist_ok=True)
        env=dict(os.environ);env.pop('DKRR_PS5_ENV',None)
        if rules:env['DKRR_PS5_ENV']=rules
        out=subprocess.run([str(work/'probe'),str(folder)],env=env,capture_output=True,text=True,check=True).stdout
        return dict(line.split('=',1) for line in out.splitlines())
    base=run('')
    assert base['proc']=='1' and base['uid_is_owner']=='1' and base['flock']=='1' and base['chmod']=='1' and base['fsync']=='1',base
    assert base['nlink']=='1' and base['hard_link_count']=='1' and base['fchmod']=='1',base
    for rule,(key,before,after) in EXPECT.items():
        seen=run(rule)
        assert seen[key]==after,(rule,seen)
        others={k:v for k,v in seen.items() if k not in (key,'hard_link_count')}
        untouched={k:v for k,v in base.items() if k not in (key,'hard_link_count')}
        assert others==untouched,('rule leaks into other behaviour',rule,others)
    assert run('nlink0')['hard_link_count']=='0','the standard library must see the zero link count'
    assert run('chmod')['fchmod']=='1','fchmod stays available: only chmod/fchmodat are modelled'
    everything=run('all')
    assert all(everything[k]=='0' for k in ('proc','nlink','uid_is_owner','flock','chmod','fsync')) and everything['fchmod']=='1',everything
    assert run('nlink0,flock')['flock']=='0' and run('nlink0,flock')['nlink']=='0'
print('PASS PS5 host environment rules: each isolated, combined, off by default')
