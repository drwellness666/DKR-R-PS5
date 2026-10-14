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
