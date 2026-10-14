#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual source update digest/extraction/API request logic with offline fixtures."""
import hashlib,io,json,pathlib,stat,tempfile,zipfile
from update_source_release import ASSET,REPO,select_release,extract_source,request,Redirect
from urllib.request import Request
with tempfile.TemporaryDirectory(prefix='dkrr-release-test-') as tmp:
 t=pathlib.Path(tmp)
 def zipdata(extra=()):
  stream=io.BytesIO()
  with zipfile.ZipFile(stream,'w') as z:
   for name in ('README.md','GAME_SOURCES.json','tools/check_public_source.py','build_game.sh'):z.writestr('DKR-R-PS5/'+name,b'source fixture')
   for name,data in extra:z.writestr(name,data)
  return stream.getvalue()
 data=zipdata();digest=hashlib.sha256(data).hexdigest()
 row={'tag_name':'ps5-beta.1','prerelease':True,'assets':[{'name':ASSET,'state':'uploaded','size':len(data),'digest':'sha256:'+digest,'url':'https://api.github.com/repos/'+REPO+'/releases/assets/42'}]}
 assert select_release([row],'stable') is None
 release=select_release([row],'beta');extract_source(data,release,t/'valid');assert (t/'valid/README.md').read_bytes()==b'source fixture'
 try:extract_source(data+b'bad',release,t/'corrupt');raise AssertionError('Tampering accepted')
 except ValueError:assert not (t/'corrupt').exists()
 for i,name in enumerate(('DKR-R-PS5/../outside','/outside','DKR-R-PS5/work/private/ROM.n64','DKR-R-PS5/.git/config','DKR-R-PS5/nested/private/item.txt','DKR-R-PS5/game.n64')):
  bad=zipdata(((name,b'bad'),));spec=dict(release,size=len(bad),sha256=hashlib.sha256(bad).hexdigest())
  try:extract_source(bad,spec,t/('bad'+str(i)));raise AssertionError('Unsafe source accepted')
  except ValueError:assert not (t/('bad'+str(i))).exists()
 row['assets'][0]['digest']='missing'
 try:select_release([row],'beta');raise AssertionError('Missing digest accepted')
 except ValueError:pass
 assert request('https://api.github.com/repos/'+REPO+'/releases',fetch=lambda req,limit:b'[]')==b'[]'
 req=Request('https://api.github.com/repos/'+REPO+'/releases/assets/42',headers={'Authorization':'Bearer synthetic-test-only'})
 redirected=Redirect().redirect_request(req,None,302,'Found',{},'https://release-assets.githubusercontent.com/synthetic')
 assert redirected.get_header('Authorization') is None
 print('PASS actual source updater: beta/stable selection, GitHub digest, tamper/traversal/private/git refusal and no authorization forwarding on cross-origin redirects; no network/build/console')
