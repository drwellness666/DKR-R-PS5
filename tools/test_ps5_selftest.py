#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The self-test must record what the system really does, survive refusals, and never loop on its own crash."""
import json,os,pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
PROGRAM=r'''
#include "ps5_selftest.hpp"
int main(int,char**argv){
    setenv("TMPDIR",argv[2],1);
    const auto results=dkrr::selftest::run(argv[1],"integrated-test",argv[3],argv[4]);
    return results.empty()?3:0;
}
'''
def by_name(path):
    data=json.loads(path.read_text());assert data['schema']==1 and data['build']=='integrated-test'
    return {r['name']:r for r in data['results']}
with tempfile.TemporaryDirectory(prefix='dkrr-selftest-') as temp:
    work=pathlib.Path(temp);(work/'main.cpp').write_text(PROGRAM)
    for c in ('shim.o','diag.o'):pass
    subprocess.run(['clang','-std=c11','-O1','-c',str(root/'tests/ps5_env_shim.c'),'-o',str(work/'shim.o')],check=True)
    subprocess.run(['clang','-std=c11','-O1','-I',str(root/'src'),'-DDKR_DIAG_DATA_ROOT="'+str(work/'journal')+'"','-c',str(root/'src/ps5_diagnostics.c'),'-o',str(work/'diag.o')],check=True)
    subprocess.run(['clang++','-std=c++20','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root/'src'),str(work/'main.cpp'),str(work/'diag.o'),str(work/'shim.o'),'-ldl','-o',str(work/'selftest')],check=True)
    def start(name,rules='',pre=None):
        profile=work/name;(profile).mkdir();tmp=work/(name+'-tmp');tmp.mkdir();app=work/(name+'-app');app.mkdir();(app/'eboot.bin').write_text('x')
        data=work/(name+'-data');data.mkdir();os.chmod(data,0o555)
        if pre:pre(profile)
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('DKRR_PS5_ENV',None)
        if rules:env['DKRR_PS5_ENV']=rules
        done=subprocess.run([str(work/'selftest'),str(profile),str(tmp),str(app),str(data)],env=env,capture_output=True,text=True)
        os.chmod(data,0o755);return profile,done
    # Plain host: everything the host allows is allowed.
    profile,done=start('plain');assert done.returncode==0,done.stderr
    plain=by_name(profile/'logs/selftest.json')
    assert plain['flock exclusive']['ok'] and plain['flock excludes second holder']['ok'] and plain['chmod']['ok'] and plain['fsync']['ok']
    assert plain['readlink /proc/self/exe']['ok'] and 'nlink=1' in plain['stat file']['detail']
    assert plain['create folder in data root']['ok'] is False and 'errno=13' in plain['create folder in data root']['detail'] or os.geteuid()==0,plain['create folder in data root']
    assert plain['TMPDIR set']['ok'] and plain['crash journal open']['ok'] is False  # no journal was opened in this process
    assert plain['default thread stack size']['ok'] and plain['default thread stack size']['detail'].startswith('bytes='),plain['default thread stack size']
    assert not (profile/'selftest').exists() and not (profile/'logs/selftest.running').exists(),'files must be cleaned up'
    assert oct((profile/'logs/selftest.json').stat().st_mode & 0o777)=='0o666'
    # Under every PS5 rule each refusal is recorded with its errno and the run still completes.
    profile,done=start('ps5','all');assert done.returncode==0,done.stderr
    ps5=by_name(profile/'logs/selftest.json')
    assert 'errno=95' in ps5['flock exclusive']['detail'],ps5['flock exclusive']
    assert 'errno=2' in ps5['readlink /proc/self/exe']['detail'] and 'errno=38' in ps5['chmod']['detail'] and 'errno=38' in ps5['fchmodat']['detail']
    assert ps5['fchmod']['ok'] and 'errno=22' in ps5['fsync']['detail'] and 'nlink=0' in ps5['stat file']['detail'] and 'uid=0' in ps5['stat file']['detail']
    assert 'flock excludes second holder' not in ps5,'no second-holder probe when the lock itself is refused'
    # A previous crash inside the self-test: skip once, say so, then run again.
    def crashed(profile):(profile/'logs').mkdir();(profile/'logs/selftest.running').write_text('')
    profile,done=start('crash',pre=crashed);assert done.returncode==3  # no results: skipped
    assert (profile/'logs/selftest-skipped.txt').exists() and not (profile/'logs/selftest.running').exists() and not (profile/'logs/selftest.json').exists()
    done=subprocess.run([str(work/'selftest'),str(profile),str(work/'crash-tmp'),str(work/'crash-app'),str(work/'crash-data')],capture_output=True)
    assert done.returncode==0 and (profile/'logs/selftest.json').exists(),'the next start runs the self-test again'
print('PASS self-test: measured facts, PS5 refusals recorded with errno, cleanup, one-time skip after its own crash')
