#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual combined updater/rollback, all-stage refusal and resume; synthetic IO."""
import hashlib,json,pathlib,subprocess,tarfile,tempfile
root=pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='dkrr-combined-') as folder:
 t=pathlib.Path(folder);dest=t/'app';metas=[dest/'sce_sys',t/'registered',t/'appmeta']
 artifacts={'assets/notice.txt':(b'old notice',b'new notice'),'eboot.bin':(b'old fake code',b'new fake code')}
 art={n:('recorded-'+n).encode() for n in ('param.json','icon0.png','pic0.dds','pic1.dds')}
 with tarfile.open(root/'vendor/ps5-sdk-source.tar.gz') as tar:sha=tar.extractfile('crt/nid.c').read().decode()
 sha=sha.replace('nid_encode(','dkrr_unused_nid_encode(')+'''\n#include <unistd.h>
#include <errno.h>
int dkrr_sha1_fd(int fd,unsigned char digest[20]){SHA1_CTX c;unsigned char b[8192];ssize_t n;SHA1Init(&c);for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}SHA1Final(digest,&c);return 0;}\n'''
 (t/'sha1.c').write_text(sha)
 subprocess.run(['clang','-D_GNU_SOURCE','-O1','-fsanitize=address,undefined','-c',str(t/'sha1.c'),'-o',str(t/'sha1.o')],check=True)
 for keep,restore in ((False,False),(False,True),(True,False),(True,True)):
  build=t/(('paired-' if keep else '')+('rollback' if restore else 'forward'));build.mkdir()
  header='struct update_file {const char*path;const unsigned char*start;unsigned long long old_size,new_size;const char*old_sha1,*new_sha1;unsigned mode;};\n';rows=[]
  for index,(name,pair) in enumerate(artifacts.items()):
   old,new=pair[::-1] if restore else pair
   header+=f'static const unsigned char bytes_{index}[]={{'+','.join(str(n) for n in new)+'};\n'
   rows.append('{'+json.dumps(str(dest/name))+f',bytes_{index},{len(old)},{len(new)},"{hashlib.sha1(old).hexdigest()}","{hashlib.sha1(new).hexdigest()}",0644'+'}')
  header+='static const struct update_file update_files[]={'+','.join(rows)+'};\n';(build/'native_update_files.h').write_text(header)
  header='struct poster_file {const char*path,*backup;unsigned long long size;const char*sha1;};\n'
  def row(meta,name,backup=False):
   path=str(meta/name);backup_name=json.dumps(path+'.dkrr-retired-launch-poster') if backup else '0'
   return '{'+json.dumps(path)+','+backup_name+f',{len(art[name])},"{hashlib.sha1(art[name]).hexdigest()}"'+'}'
  header+='static const struct poster_file poster_guards[]={'+','.join(row(m,n) for m in metas for n in ('param.json','icon0.png','pic0.dds'))+'};\n'
  header+='static const struct poster_file poster_files[]={'+','.join(row(m,'pic1.dds',True) for m in metas)+'};\n';(build/'native_poster_retirement.h').write_text(header)
  subprocess.run(['clang','-std=c11','-D_GNU_SOURCE','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DDKR_UPDATE_VERSION=23','-DDKR_RESTORE_POSTER='+str(int(restore)),'-DDKR_KEEP_POSTER='+str(int(keep)),'-I',str(build),str(root/'src/update_game_preview.c'),str(t/'sha1.o'),'-o',str(build/'test')],check=True)
 def reset():
  for m in metas:
   m.mkdir(parents=True,exist_ok=True)
   for p in m.iterdir():p.unlink()
   for n,data in art.items():(m/n).write_bytes(data)
  for n,pair in artifacts.items():
   path=dest/n;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(pair[0])
 def snapshot():return {str(p.relative_to(t)):p.read_bytes() for base in [dest,t/'registered',t/'appmeta'] for p in base.rglob('*') if p.is_file()}
 def run(restore=False,ok=True,keep=False):
  result=subprocess.run([str(t/(('paired-' if keep else '')+('rollback' if restore else 'forward'))/'test')],text=True,capture_output=True)
  assert (result.returncode==0)==ok,result.stdout+result.stderr
  if ok:assert 'DKRR PREVIEW '+('ROLLBACK' if restore else 'UPDATE')+' PASS:' in result.stdout
 reset();original=snapshot();run();run()
 assert all((dest/n).read_bytes()==pair[1] for n,pair in artifacts.items())
 assert all(not (m/'pic1.dds').exists() for m in metas)
 run(True);run(True);assert snapshot()==original
 reset();(metas[2]/'param.json').write_bytes(b'wrong registered identity');before=snapshot();run(ok=False);assert snapshot()==before
 reset();(dest/'eboot.bin').write_bytes(b'unknown game');before=snapshot();run(ok=False);assert snapshot()==before
 reset();temporary=dest/'eboot.bin.dkrr-v23-update';temporary.write_bytes(b'existing interrupted/foreign temporary');before=snapshot();run(ok=False);assert snapshot()==before;temporary.unlink()
 reset();(dest/'assets/notice.txt').write_bytes(artifacts['assets/notice.txt'][1]);(metas[0]/'pic1.dds').rename(metas[0]/'pic1.dds.dkrr-retired-launch-poster');run();run(True)
 assert all((dest/n).read_bytes()==pair[0] for n,pair in artifacts.items())
 reset();(metas[2]/'pic1.dds').unlink();before=snapshot();run(True,False);assert snapshot()==before
 reset();original=snapshot();run(keep=True);run(keep=True);run(True,keep=True);run(True,keep=True);assert snapshot()==original
 reset();(metas[2]/'pic1.dds').write_bytes(b'unknown launch poster');before=snapshot();run(ok=False,keep=True);assert snapshot()==before
 print('PASS actual combined update/rollback and retained paired art: exact return to baseline, repeat/resume, complete preflight and no changes on unknown app/metadata/temp or missing restore backup')
