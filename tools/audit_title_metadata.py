#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read only PPSA94678 presentation fields from a local app.db copy; never update."""
import argparse,json,pathlib,sqlite3
TITLE='PPSA94678'
def audit(path):
 if path.is_symlink() or not path.is_file():raise ValueError('Use an ordinary local database copy')
 db=sqlite3.connect(path.resolve().as_uri()+'?mode=ro',uri=True)
 try:
  db.execute('PRAGMA query_only=ON')
  columns=[row[1] for row in db.execute('PRAGMA table_info(tbl_contentinfo)')]
  by_lower={name.lower():name for name in columns}
  if 'titleid' not in by_lower:raise ValueError('Unknown tbl_contentinfo schema')
  fields=[by_lower[key] for key in ('titleid','metadataPath'.lower(),'icon0info','pic0info','pic1info') if key in by_lower]
  quote=lambda value:'"'+value.replace('"','""')+'"'
  rows=db.execute('SELECT '+','.join(quote(f) for f in fields)+' FROM tbl_contentinfo WHERE '+quote(by_lower['titleid'])+'=?',(TITLE,)).fetchall()
  result={'title_id':TITLE,'read_only':True,'rows':[dict(zip(fields,row)) for row in rows],
          'missing_schema_fields':[key for key in ('pic0info','pic1info') if key not in by_lower],
          'limit':'Only local own-title database fields; file copies do not establish Shell registration or visible acceptance.'}
  return result
 finally:db.close()
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('database',type=pathlib.Path);args=parser.parse_args()
 try:print(json.dumps(audit(args.database),indent=2))
 except (ValueError,sqlite3.DatabaseError) as e:raise SystemExit(str(e))
