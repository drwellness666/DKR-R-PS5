#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,pathlib,sqlite3,tempfile
from audit_title_metadata import audit
with tempfile.TemporaryDirectory(prefix='dkrr-own-art-db-') as directory:
 path=pathlib.Path(directory)/'app.db';db=sqlite3.connect(path)
 db.execute('CREATE TABLE tbl_contentinfo(titleId TEXT,metaDataPath TEXT,icon0Info TEXT,pic0Info TEXT,pic1Info TEXT,unrelated TEXT)')
 db.executemany('INSERT INTO tbl_contentinfo VALUES(?,?,?,?,?,?)',[
 ('PPSA94678','/user/appmeta/PPSA94678','icon0.png','','pic1.dds','OWN_PRIVATE_UNUSED'),
 ('OTHER','secret-path','secret-icon','secret-picture','secret-poster','SECRET_OTHER_ROW')])
 db.commit();db.close();before=hashlib.sha256(path.read_bytes()).hexdigest()
 result=audit(path);text=str(result)
 assert len(result['rows'])==1 and result['rows'][0]['pic0Info']==''
 assert 'secret' not in text.lower() and 'OWN_PRIVATE_UNUSED' not in text
 assert hashlib.sha256(path.read_bytes()).hexdigest()==before
 print('PASS: read-only own-title field audit, exact file preservation and other-title/private-column exclusion')

# Exercise the exact native metadata builder, rather than a parallel formatter.
import json,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[1]
script=(root/'tools/link_native_game.sh').read_text()
at=script.index('python3 - "$support/sce_sys/param-radv.json"')
body=script[script.index('\n',at)+1:script.index('\nPY\n',at)]
with tempfile.TemporaryDirectory(prefix='dkrr-title-version-') as directory:
 folder=pathlib.Path(directory);template=folder/'template.json';release=folder/'release.json'
 fixture={'titleId':'TEMPLATE','localizedParameters':{'defaultLanguage':'en-US','en-US':{'titleName':'Template'}},'attribute':32,'attribute3':17,'other':{'retain':True}}
 template.write_text(json.dumps(fixture))
 for index,(tag,wanted) in enumerate((('ps5-v1.0.0','01.000.000'),('ps5-v1.2.3','01.000.000'),('ps5-v99.999.999','01.000.000'))):
  release.write_text(json.dumps({'tag':tag}))
  for high in ('0','1'):
   output=folder/f'valid-{index}-{high}.json'
   subprocess.run([sys.executable,'-',str(template),str(output),str(release),high],input=body,text=True,check=True)
   actual=json.loads(output.read_text());assert actual['contentVersion']==wanted and actual['localizedParameters']['en-US']['titleName']=='DKR-R PS5'
   assert actual['titleId']=='PPSA94678' and actual['contentId']=='UP9000-PPSA94678_00-DKRRNATIVE000001'
   assert actual['attribute']==32|0x20000000 and actual['attribute3']==(17|0x40040 if high=='1' else 17) and actual['other']==fixture['other']
 for index,tag in enumerate(('ps5-v100.0.0','ps5-v1.1000.0','ps5-v1.0.1000','ps5-v1.0.0\n','ps5-beta.3.4','../ps5-v1.0.0')):
  release.write_text(json.dumps({'tag':tag}));output=folder/f'invalid-{index}.json'
  run=subprocess.run([sys.executable,'-',str(template),str(output),str(release),'1'],input=body,text=True,capture_output=True)
  assert run.returncode!=0 and not output.exists()
 print('PASS actual native title metadata: stable home name and registered version across release tags, retained identifiers/capabilities and invalid tag refusal')
