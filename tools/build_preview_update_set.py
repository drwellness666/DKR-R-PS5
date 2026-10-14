#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build one exact-predecessor update and its rollback; local private outputs only."""
import argparse,hashlib,json,pathlib,subprocess
from build_native_game_bundle_updater import make_plan
TITLE='PPSA94678'
DESTINATIONS=('/data/homebrew/PPSA94678/sce_sys','/user/app/PPSA94678/sce_sys','/user/appmeta/PPSA94678')
def build(sdk,gpu,previous,current,output,expected,sequence=23):
 root=pathlib.Path(__file__).resolve().parent.parent
 plan,old,new=make_plan(previous,current)
 if expected!=plan['previous_manifest_sha256']:raise ValueError('Recorded predecessor manifest differs')
 if plan['new_files_for_installer']:raise ValueError('Combined update requires an existing complete own application')
 for name in ('param.json','icon0.png','pic0.dds'):
  if (previous/'sce_sys'/name).read_bytes()!=(current/'sce_sys'/name).read_bytes():raise ValueError('Combined scope requires unchanged title identity/icon/home-menu art')
 data={n:(previous/'sce_sys'/n).read_bytes() for n in ('param.json','icon0.png','pic0.dds','pic1.dds')}
 if data['pic1.dds']!=data['pic0.dds']:raise ValueError('Unknown previous launch art')
 keep_poster=(current/'sce_sys/pic1.dds').is_file()
 if keep_poster and (current/'sce_sys/pic1.dds').read_bytes()!=data['pic1.dds']:raise ValueError('Combined update requires unchanged paired artwork')
 if 'private' not in output.parts or output.exists():raise ValueError('Use a new private output directory')
 if not 1<=sequence<9999:raise ValueError('Invalid update sequence')
 output.mkdir(parents=True,mode=0o700)
 sha=(gpu/'PS5_PayloadSDK/crt/nid.c').read_text().replace('nid_encode(','dkrr_unused_nid_encode(')+'''\n#include <errno.h>
extern long read(int,void*,unsigned long);
int dkrr_sha1_fd(int fd,unsigned char digest[20]){SHA1_CTX c;unsigned char b[8192];long n;SHA1Init(&c);for(;;){n=read(fd,b,sizeof(b));if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;SHA1Update(&c,b,(unsigned)n);}SHA1Final(digest,&c);return 0;}\n'''
 (output/'sha1.c').write_text(sha);cc=str(sdk/'bin/prospero-clang')
 subprocess.run([cc,'-O2','-c',str(output/'sha1.c'),'-o',str(output/'sha1.o')],check=True)
 payloads={}
 for restore in (False,True):
  name='rollback' if restore else 'update';work=output/(name+'-source');work.mkdir(mode=0o700)
  source_app=previous if restore else current;before,after=(new,old) if restore else (old,new)
  header='struct update_file {const char*path;const unsigned char*start;unsigned long long old_size,new_size;const char*old_sha1,*new_sha1;unsigned mode;};\n';assembly='.section .rodata\n';rows=[]
  for index,relative in enumerate(plan['preflight_files']):
   symbol='0'
   if relative in plan['changed_files']:
    symbol=f'preview_{index}';header+=f'extern const unsigned char {symbol}[];\n';assembly+=f'.balign 16\n.global {symbol}\n{symbol}:\n.incbin "{source_app/relative}"\n'
   rows.append('{'+json.dumps('/data/homebrew/'+TITLE+'/'+relative)+f',{symbol},{before[relative]["size"]},{after[relative]["size"]},"{before[relative]["sha1"]}","{after[relative]["sha1"]}",{after[relative]["mode"]:04o}'+'}')
  header+='static const struct update_file update_files[]={'+','.join(rows)+'};\n';(work/'native_update_files.h').write_text(header);(work/'embedded.S').write_text(assembly)
  header='struct poster_file {const char*path,*backup;unsigned long long size;const char*sha1;};\n'
  def entry(folder,file,backup=False):
   path=folder+'/'+file;second=json.dumps(path+'.dkrr-retired-launch-poster') if backup else '0'
   return '{'+json.dumps(path)+','+second+f',{len(data[file])},"{hashlib.sha1(data[file]).hexdigest()}"'+'}'
  header+='static const struct poster_file poster_guards[]={'+','.join(entry(d,f) for d in DESTINATIONS for f in ('param.json','icon0.png','pic0.dds'))+'};\n'
  header+='static const struct poster_file poster_files[]={'+','.join(entry(d,'pic1.dds',True) for d in DESTINATIONS)+'};\n';(work/'native_poster_retirement.h').write_text(header)
  subprocess.run([cc,'-c',str(work/'embedded.S'),'-o',str(work/'embedded.o')],check=True)
  target=output/(name+'.elf')
  subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DDKR_RESTORE_POSTER='+str(int(restore)),'-DDKR_KEEP_POSTER='+str(int(keep_poster)),'-DDKR_UPDATE_VERSION='+str(sequence+int(restore)),'-I',str(work),str(root/'src/update_game_preview.c'),str(work/'embedded.o'),str(output/'sha1.o'),'-o',str(target)],check=True)
  subprocess.run([str(sdk/'bin/prospero-strip'),'--strip-debug',str(target)],check=True)
  payloads[name]=dict(file=target.name,size=target.stat().st_size,sha256=hashlib.sha256(target.read_bytes()).hexdigest())
 manifest=dict(schema=1,title_id=TITLE,payloads=payloads,previous_manifest_sha256=plan['previous_manifest_sha256'],current_manifest_sha256=plan['current_manifest_sha256'],changed_files=plan['changed_files'],console_executed=False,paired_artwork_retained=keep_poster,scope='Private own installation update and exact rollback; no ROM/profile/save/IPC/mount/database operations. Close own title before sending. Per-file changes, not a bundle transaction.')
 (output/'READY.json').write_text(json.dumps(manifest,indent=2)+'\n')
 return manifest
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('sdk','gpu_work','previous','current','output'):p.add_argument(name,type=pathlib.Path)
 p.add_argument('--expected-previous-manifest-sha256',required=True);p.add_argument('--sequence',type=int,default=23);a=p.parse_args()
 if any(not getattr(a,n).is_absolute() for n in ('sdk','gpu_work','previous','current','output')):p.error('Use absolute paths')
 try:result=build(a.sdk,a.gpu_work,a.previous,a.current,a.output,a.expected_previous_manifest_sha256,a.sequence)
 except ValueError as e:p.error(str(e))
 print('Private update/rollback prepared:',a.output)
 for name,row in result['payloads'].items():print(name,'bytes=',row['size'],'SHA256=',row['sha256'])
 print('No console connection or operation. Never distribute these ROM-derived private binaries.')
