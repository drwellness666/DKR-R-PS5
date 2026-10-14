#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check/download a GitHub source release; never upload ROMs or deploy a title."""
import argparse,hashlib,io,json,os,pathlib,re,stat,urllib.parse,urllib.request,zipfile
REPO='drwellness666/DKR-R-PS5'
ASSET='DKR-R-PS5-source.zip'
LIMIT=64*1024*1024
class Redirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  target=urllib.parse.urlparse(newurl)
  if target.scheme!='https':raise ValueError('Non-HTTPS release redirect refused')
  result=super().redirect_request(req,fp,code,msg,headers,newurl)
  if target.netloc!=urllib.parse.urlparse(req.full_url).netloc:
   result.remove_header('Authorization')
  return result

def request(url,accept='application/vnd.github+json',fetch=None):
 parsed=urllib.parse.urlparse(url)
 if parsed.scheme!='https' or parsed.hostname not in ('api.github.com','github.com'):
  raise ValueError('Use the fixed GitHub release origin')
 headers={'Accept':accept,'User-Agent':'DKR-R-PS5-source-update','X-GitHub-Api-Version':'2026-03-10'}
 token=os.environ.get('DKR_GITHUB_TOKEN')
 if token and parsed.hostname=='api.github.com':headers['Authorization']='Bearer '+token
 req=urllib.request.Request(url,headers=headers)
 if fetch is not None:return fetch(req,LIMIT)
 with urllib.request.build_opener(Redirect()).open(req,timeout=60) as response:
  data=response.read(LIMIT+1)
 if len(data)>LIMIT:raise ValueError('Release response exceeds source size limit')
 return data

def select_release(rows,channel):
 if not isinstance(rows,list):raise ValueError('Expected GitHub release list')
 for row in rows:
  if row.get('draft') or (row.get('prerelease') and channel=='stable'):continue
  tag=row.get('tag_name','')
  if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]{0,79}',tag):raise ValueError('Invalid release tag')
  assets=[a for a in row.get('assets',[]) if a.get('name')==ASSET and a.get('state')=='uploaded']
  if len(assets)!=1:continue
  asset=assets[0];digest=asset.get('digest','')
  if not re.fullmatch(r'sha256:[0-9a-f]{64}',digest):raise ValueError('Release needs the GitHub SHA256 asset digest')
  size=asset.get('size')
  if not isinstance(size,int) or not 0<size<=LIMIT:raise ValueError('Invalid source asset size')
  expected='https://api.github.com/repos/'+REPO+'/releases/assets/'
  if not re.fullmatch(re.escape(expected)+r'[0-9]+',asset.get('url','')):raise ValueError('Foreign release asset refused')
  return dict(tag=tag,asset_url=asset['url'],size=size,sha256=digest[7:],prerelease=bool(row.get('prerelease')))
 return None

def extract_source(data,release,output):
 if len(data)!=release['size'] or hashlib.sha256(data).hexdigest()!=release['sha256']:
  raise ValueError('Source asset checksum/size mismatch')
 if not output.is_absolute() or output.exists():raise ValueError('Use a new absolute source directory')
 with zipfile.ZipFile(io.BytesIO(data)) as archive:
  members=archive.infolist();names=set();total=0
  if not 1<=len(members)<=15000:raise ValueError('Invalid source member count')
  for member in members:
   path=pathlib.PurePosixPath(member.filename)
   if path.is_absolute() or '..' in path.parts or '\\' in member.filename or ':' in member.filename or not path.parts or path.parts[0]!='DKR-R-PS5' or '.git' in path.parts or path.parts[1:2]==('work',) or 'private' in path.parts:raise ValueError('Unsafe source archive path')
   if path.suffix.lower() in {'.z64','.v64','.n64','.o2r','.mpk','.eep','.sra','.fla','.elf','.prx','.bin','.o','.a'}:raise ValueError('Private game/binary source member refused')
   if not member.is_dir() and archive.open(member).read(4) in {bytes.fromhex(x) for x in ('80371240','37804012','40123780')}:raise ValueError('ROM source member refused')
   mode=member.external_attr>>16
   if mode and stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR):raise ValueError('Source links/special entries refused')
   if member.filename in names:raise ValueError('Duplicate source entry')
   names.add(member.filename);total+=member.file_size
   if total>192*1024*1024:raise ValueError('Expanded source limit exceeded')
  required={'DKR-R-PS5/README.md','DKR-R-PS5/GAME_SOURCES.json','DKR-R-PS5/tools/check_public_source.py','DKR-R-PS5/build_game.sh'}
  if not required<=names:raise ValueError('Incomplete integration source asset')
  output.mkdir(parents=True,mode=0o700)
  for member in members:
   relative=pathlib.PurePosixPath(member.filename).parts[1:]
   if not relative:continue
   target=output.joinpath(*relative)
   if member.is_dir():target.mkdir(parents=True,exist_ok=True);continue
   target.parent.mkdir(parents=True,exist_ok=True)
   with archive.open(member) as source,target.open('xb') as dest:
    while part:=source.read(65536):dest.write(part)
   target.chmod(0o644) # recipes are explicitly invoked through Bash/Python
 return output

def main(argv=None):
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--channel',choices=('stable','beta'),default='beta');p.add_argument('--download',type=pathlib.Path)
 a=p.parse_args(argv)
 try:
  rows=json.loads(request('https://api.github.com/repos/'+REPO+'/releases?per_page=30'))
  release=select_release(rows,a.channel)
  if release is None:print('No matching source release is available. No build or console action.');return 0
  local=pathlib.Path(__file__).resolve().parent.parent/'PORT_RELEASE.json'
  current=json.loads(local.read_text()).get('tag') if local.is_file() else None
  print('Release matches this source checkout.' if current==release['tag'] else 'A different source release is available; review qualification before building.')
  print('Source release:',release['tag'],'SHA256:',release['sha256'],'prerelease:',release['prerelease'])
  if a.download:
   if 'private' not in a.download.parts:raise ValueError('Use a private local source update directory')
   data=request(release['asset_url'],'application/octet-stream');extract_source(data,release,a.download)
   print('Verified source downloaded:',a.download,'; no build, ROM upload or console connection.')
  return 0
 except Exception as e:
  # Headers/tokens/redirect query strings are deliberately not included in logs.
  print('STOP: source update refused ('+type(e).__name__+'). Check release access, digest and archive constraints.');return 1
if __name__=='__main__':raise SystemExit(main())
