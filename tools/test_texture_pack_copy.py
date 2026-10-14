#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the real own-pack copier with the pinned SDK SHA1 and temporary files."""
import hashlib, os, pathlib, subprocess, tarfile, tempfile

root = pathlib.Path(__file__).resolve().parent.parent
archive = root / 'vendor/ps5-sdk-source.tar.gz'
expected = next(line.split()[0] for line in (root/'vendor/SHA256SUMS').read_text().splitlines()
                if line.split()[1] == 'ps5-sdk-source.tar.gz')
assert hashlib.sha256(archive.read_bytes()).hexdigest() == expected
with tarfile.open(archive) as tar:
    sha = tar.extractfile('crt/nid.c').read().decode()
at = sha.index('{', sha.index('SHA1Final(')) + 1
depth = 1
while depth:
    depth += (sha[at] == '{') - (sha[at] == '}')
    at += 1
sha = sha[:at] + '''
#include <errno.h>
extern long read(int,void*,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]) {
 SHA1_CTX c; unsigned char b[262144]; long n; SHA1Init(&c);
 for (;;) { n=read(fd,b,sizeof(b)); if(n<0&&errno==EINTR)continue;
  if(n<0)return -1; if(!n)break; SHA1Update(&c,b,(unsigned)n); }
 SHA1Final(digest,&c); return 0;
}
'''
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='dkrr-pack-copy-') as tmp:
    work=pathlib.Path(tmp); data=work/'data'; data.mkdir()
    old=data/'profile-us-v77/texture-packs'; old.mkdir(parents=True)
    new=data/'profile-us-v80'; new.mkdir()
    # Multiple chunks, partial final chunk and zeros exercise the actual hash/copy path.
    contents=bytes(range(256))*2049+b'owned texture fixture\0end'
    name='own-pack.zip'; source=old/name; source.write_bytes(contents)
    preserved=[]
    for profile in (old.parent,new):
        for leaf in ('controls.json','save.mpk'):
            file=profile/leaf; file.write_bytes(b'preserve '+leaf.encode()); file.chmod(0o600)
            preserved.append((file,file.read_bytes(),file.stat().st_mtime_ns,file.stat().st_mode))
    (work/'sha.c').write_text(sha)
    flags=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
    subprocess.run(flags+['-Wno-unused-function','-c',str(work/'sha.c'),'-o',str(work/'sha.o')],check=True)
    subprocess.run(flags+['-DDKR_TEXTURE_DATA_ROOT="'+str(data)+'"',
        '-DDKR_TEXTURE_UID='+str(os.geteuid()),'-DDKR_PACK_NAME="'+name+'"',
        '-DDKR_PACK_SHA1="'+hashlib.sha1(contents).hexdigest()+'"',
        '-DDKR_PACK_BYTES='+str(len(contents))+'ULL',str(root/'src/copy_owned_texture_pack.c'),
        str(work/'sha.o'),'-o',str(work/'copy')],check=True)
    def run(code):
        result=subprocess.run([str(work/'copy')],capture_output=True,env=env)
        assert result.returncode == code, result.stdout+result.stderr
        assert b'Sanitizer' not in result.stderr, result.stderr
        for file,blob,mtime,mode in preserved:
            assert (file.read_bytes(),file.stat().st_mtime_ns,file.stat().st_mode)==(blob,mtime,mode)
        assert source.read_bytes()==contents
        return result
    target=new/'texture-packs'/name
    run(0); assert target.read_bytes()==contents and target.stat().st_mode&0o777==0o644
    before=target.stat().st_mtime_ns; run(0); assert target.stat().st_mtime_ns==before
    target.write_bytes(b'other user pack'); run(6); assert target.read_bytes()==b'other user pack'
    target.unlink(); target.symlink_to(source); run(6); assert target.is_symlink()
    target.unlink(); temporary=target.with_name(name+'.dkrr-copying'); temporary.write_bytes(b'prior partial')
    run(7); assert temporary.read_bytes()==b'prior partial' and not target.exists(); temporary.unlink()
    source.write_bytes(contents[:-1]+b'!')
    result=subprocess.run([str(work/'copy')],capture_output=True,env=env)
    assert result.returncode==3 and not target.exists(); source.write_bytes(contents)
    target.parent.rmdir(); target.parent.symlink_to(old); run(5); assert target.parent.is_symlink()
    target.parent.unlink(); source.unlink(); source.symlink_to(preserved[0][0])
    result=subprocess.run([str(work/'copy')],capture_output=True,env=env)
    assert result.returncode==3; source.unlink(); source.write_bytes(contents)
    new.rename(data/'profile-saved'); result=subprocess.run([str(work/'copy')],capture_output=True,env=env)
    assert result.returncode==4 and not new.exists()
print('PASS actual pack copier: multi-chunk/hash/read-back, idempotency, unchanged settings/saves, foreign target, symlinks, prior temporary and missing profile refusal')
