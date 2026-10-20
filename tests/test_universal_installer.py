#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual universal C installer with synthetic ROM banks, filesystem and faults."""
import fcntl,hashlib,json,os,pathlib,shutil,struct,subprocess,sys,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from build_universal_installer import emit,hash_source,release_files
from build_native_game_bundle_updater import CORE,inventory,inventory_hash
SIZE=12*1024*1024
SOUNDS=(0x21,0x232,0x233,0x135,0x134,0x151,0x0f,0x9f,0x13a,*range(0x156,0x160),0xa0,0xa1,0xa2,0x23d,0x107)
def fixture_rom(revision):
    rom=bytearray(SIZE);rom[:4]=bytes.fromhex('80371240');base=969568 if revision==77 else 970976
    def u32(p,v):struct.pack_into('>I',rom,p,v)
    def u16(p,v):struct.pack_into('>H',rom,p,v)
    u32(base,50);start=base+208;table=start+0x100;audio=start+0x1000
    u32(base+39*4,0x100);u32(base+40*4,0x1000)
    for p,v in ((0,0x1800),(4,0x800),(8,0x1000),(16,0x5000),(24,0x2000),(28,0x4000)):u32(table+p,v)
    bank=audio+0x800;u16(bank,0x4231);u32(bank+4,0x20);u32(bank+0x24,32000);u32(bank+0x2c,0x40)
    u16(bank+0x4e,1);u32(bank+0x50,0x80)
    for off,v in ((0,0xa0),(4,0xc0),(8,0xe0)):u32(bank+0x80+off,v)
    rom[bank+0x8d]=127;u32(bank+0xa4,1000000);rom[bank+0xac:bank+0xae]=b'\x7f\x7f';rom[bank+0xc4]=60
    u32(bank+0xe4,9);u32(bank+0xf0,0x120);u32(bank+0x120,2);u32(bank+0x124,1)
    rom[audio+0x1000:audio+0x1009]=bytes.fromhex('60 12 34 56 78 9a bc de f0')
    for sound in SOUNDS:
        p=audio+0x2000+10*sound;u16(p,1);rom[p+2]=128;rom[p+4]=100
    # Synthetic music bank and one-note CSeq, shared by eight fixture jingles.
    u16(audio,0x4231);u32(audio+4,0x20);u16(audio+0x20,1);u32(audio+0x24,32000);u32(audio+0x2c,0x40)
    rom[audio+0x40]=127;u16(audio+0x4e,1);u32(audio+0x50,0x80)
    for off,v in ((0,0xa0),(4,0xc0),(8,0xe0)):u32(audio+0x80+off,v)
    rom[audio+0x8d]=127;u32(audio+0xa4,1000000);u32(audio+0xa8,100000);rom[audio+0xac:audio+0xae]=b'\x7f\x7f'
    rom[audio+0xc1]=127;rom[audio+0xc3]=127;rom[audio+0xc4]=60
    u32(audio+0xe4,9);u32(audio+0xf0,0x120);u32(audio+0x120,2);u32(audio+0x124,1)
    rom[audio+0x1800:audio+0x1809]=bytes.fromhex('60 12 34 56 78 9a bc de f0')
    seq=bytearray(68);struct.pack_into('>I',seq,0,68);struct.pack_into('>I',seq,64,96)
    seq+=bytes.fromhex('00 ff 51 07 a1 20 00 c0 00 00 90 3c 64 18 18 ff 2f')
    sf=audio+0x5000;u16(sf,0x5331);u16(sf+2,51)
    for i in range(43,51):u32(sf+4+8*i,0x200);u32(sf+8+8*i,len(seq))
    rom[sf+0x200:sf+0x200+len(seq)]=seq
    return bytes(rom)
class Universal(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
    cls.tmp=tempfile.TemporaryDirectory(prefix='dkrr-universal-');cls.base=pathlib.Path(cls.tmp.name);cls.data=cls.base/'data';cls.mnt=cls.base/'mnt';cls.mnt.mkdir();cls.user=cls.base/'system-user';cls.user.mkdir()
    cls.roms={r:fixture_rom(r) for r in (77,80)}
    cls.hashes={r:hashlib.sha1(data).hexdigest() for r,data in cls.roms.items()}
    (cls.base/'hash.c').write_text(hash_source());(cls.base/'fault.c').write_text('''#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
int __real_rename(const char*,const char*);
int __real_fsync(int);
ssize_t __real_read(int,void*,size_t);
ssize_t __wrap_read(int fd,void*buf,size_t n){
 char p[64],name[1024];snprintf(p,sizeof(p),"/proc/self/fd/%d",fd);ssize_t len=readlink(p,name,sizeof(name)-1);if(len>=0)name[len]=0;else name[0]=0;
 if(getenv("DKRR_REJECT_FULL_NONROM_READ")&&strstr(name,"not-a-rom.bin")&&n>4)_exit(79);
 return __real_read(fd,buf,n);
}
static int moved_to_history=0;
int __wrap_fsync(int fd){
 char p[64],name[1024];snprintf(p,sizeof(p),"/proc/self/fd/%d",fd);ssize_t len=readlink(p,name,sizeof(name)-1);if(len>=0)name[len]=0;else name[0]=0;
 if(moved_to_history&&getenv("DKRR_FAIL_HISTORY_SYNC")&&strstr(name,"/installation/history")){errno=EIO;return -1;}
 return __real_fsync(fd);
}
int __wrap_rename(const char*a,const char*b){
 if(getenv("DKRR_FAIL_ACTIVATE")&&strstr(a,"staged-integrated-")&&strstr(b,"homebrew/PPSA94678")){errno=EIO;return -1;}
 int rc=__real_rename(a,b);
 if(rc==0&&strstr(a,"homebrew/PPSA94678")&&strstr(b,"/history/"))moved_to_history=1;
 if(rc==0&&getenv("DKRR_POWER_CUT")&&strstr(a,"homebrew/PPSA94678")&&strstr(b,"/history/"))_exit(77);
 if(rc==0&&getenv("DKRR_POWER_AFTER_ACTIVATE")&&strstr(a,"staged-integrated-")&&strstr(b,"homebrew/PPSA94678"))_exit(78);
 return rc;
}
ssize_t __real_write(int,const void*,size_t);
ssize_t __wrap_write(int fd,const void*buf,size_t n){
 static int failed=0;char p[64],name[1024];snprintf(p,sizeof(p),"/proc/self/fd/%d",fd);ssize_t len=readlink(p,name,sizeof(name)-1);if(len>=0)name[len]=0;else name[0]=0;
 if(getenv("DKRR_FAIL_WRITE")&&strstr(name,"staged-integrated-")&&strstr(name,"eboot.bin")){
  if(failed){errno=ENOSPC;return -1;}failed=1;return __real_write(fd,buf,n>1?n/2:n);
 }
 return __real_write(fd,buf,n);
}
''')
    flags=['clang','-std=c11','-D_GNU_SOURCE','-g','-O1','-fsanitize=address,undefined']
    cls.flags=flags
    cls.objects=[]
    for src in (cls.base/'hash.c',*[ROOT/'installer'/n for n in ('files.c','rom.c','audio.c','vadpcm.c','register.c')]):
        out=cls.base/(src.name+'.o');warnings=['-Wall','-Wextra','-Werror'] if src.parent==ROOT/'installer' else []
        subprocess.run(flags+warnings+['-I',str(ROOT/'installer'),'-c',str(src),'-o',str(out)],check=True);cls.objects.append(out)
    cls.binaries=[];cls.legacy=cls.base/'legacy';cls.legacy_receipt=cls.base/'legacy-receipt.json'
    for name in CORE:
        p=cls.legacy/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'legacy synthetic '+name.encode())
    (cls.legacy/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+b'legacy-code')
    (cls.legacy/'sce_sys/param.json').write_text(json.dumps({'titleId':'PPSA94678','attribute3':0x40040}))
    files=inventory(cls.legacy)
    cls.legacy_receipt.write_text(json.dumps(dict(files=files,app_manifest_sha256=inventory_hash(files),build_id='integrated-'+'0'*24)))
    for i in range(3):
        app=cls.base/f'app{i}';build='integrated-'+str(i+1)*24;w=cls.base/f'build{i}';w.mkdir()
        for name in CORE:
            p=app/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'public synthetic '+name.encode())
        (app/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d')+build.encode())
        # Both installed artwork files must remain distinct entries even when
        # their package symbols share one embedded byte sequence.
        (app/'sce_sys/pic1.dds').write_bytes((app/'sce_sys/pic0.dds').read_bytes())
        (app/'sce_sys/param.json').write_text(json.dumps({'titleId':'PPSA94678','attribute3':0x40040}))
        private=app/'assets/controller-audio/us-v77';private.mkdir(parents=True);(private/'0.pcm').write_bytes(b'MUST-NOT-BE-EMBEDDED')
        files=emit(app,build,w,[(cls.legacy,cls.legacy_receipt)],sequence=i+1);assert not any('controller-audio' in n for n in files)
        subprocess.run(['clang','-c',str(w/'embedded.S'),'-o',str(w/'embedded.o')],check=True)
        import re
        symbols=subprocess.check_output(['nm','-n',str(w/'embedded.o')],text=True)
        values={name:int(value,16) for value,name in re.findall(r'^([0-9a-f]+) \w (release_[0-9]+_(?:start|end))$',symbols,re.M)}
        i0=sorted(files).index('sce_sys/pic0.dds');i1=sorted(files).index('sce_sys/pic1.dds')
        assert values[f'release_{i0}_start']==values[f'release_{i1}_start']
        assert values[f'release_{i0}_end']==values[f'release_{i1}_end']
        target=w/'installer'
        common=[f'-DDKRR_TEST_ROM_HASH77="{cls.hashes[77]}"',f'-DDKRR_TEST_ROM_HASH80="{cls.hashes[80]}"']
        # ROM hashes are compiled into rom.c, not the main unit.
        romobj=w/'rom.o';subprocess.run(flags+common+['-I',str(ROOT/'installer'),'-Wall','-Wextra','-Werror','-c',str(ROOT/'installer/rom.c'),'-o',str(romobj)],check=True)
        objs=[romobj if p.name=='rom.c.o' else p for p in cls.objects]
        command=flags+['-Wall','-Wextra','-Werror','-DDKRR_HOST_TEST=1',f'-DDKRR_UPDATE_ONLY={int(i==2)}',f'-DDKRR_INSTALL_ROOT="{cls.data}"',f'-DDKRR_USB_ROOT="{cls.mnt}"',f'-DDKRR_SYSTEM_USER_ROOT="{cls.user}"','-I',str(ROOT/'installer'),'-I',str(w),str(ROOT/'installer/main.c'),str(w/'embedded.o'),*[str(o) for o in objs],str(cls.base/'fault.c'),'-Wl,--wrap=rename','-Wl,--wrap=write','-Wl,--wrap=read','-Wl,--wrap=fsync','-lm','-o',str(target)]
        subprocess.run(command,check=True)
        if i==2:
            cls.network_binary=w/"network-update"
            network=command.copy();network[-1]=str(cls.network_binary);network.insert(1,"-DDKRR_NETWORK_UPDATE=1");subprocess.run(network,check=True)
        cls.binaries.append(target)
    cls.live=cls.data/'homebrew/PPSA94678'
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def setUp(self):
    if self.data.exists():shutil.rmtree(self.data)
    self.data.mkdir()
    for p in self.user.iterdir():
     if p.is_dir():shutil.rmtree(p)
     else:p.unlink()
    (self.data/'shadowmount').mkdir();(self.data/'shadowmount/manual.lst').write_bytes(b'# preserve notes\n/mnt/usb1/other-game\n')
    for p in self.mnt.iterdir():shutil.rmtree(p)
    folder=self.mnt/'usb0/DKRR';folder.mkdir(parents=True)
    for rev in (77,80):(folder/f'owned {rev}.z64').write_bytes(self.roms[rev])
 def run_installer(self,index=0,ok=True,**fault):
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1',**fault)
    r=subprocess.run([str(self.binaries[index])],env=env,capture_output=True,text=True)
    self.assertEqual(r.returncode==0,ok,r.stdout+r.stderr)
    if ok:self.assertIn('DKRR UNIVERSAL UPDATE PASS.' if index==2 else 'DKRR UNIVERSAL INSTALL PASS.',r.stdout)
    return r
 def network_process(self):
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
    p=subprocess.Popen([str(self.network_binary)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    self.assertEqual(p.stdout.readline(),b'DKRR UPDATE READY v1\n')
    return p
 def test_app_directory_move_with_ps5_cross_parent_permissions(self):
    work=self.base/'directory-owner-test';work.mkdir(exist_ok=True)
    code=work/'owner.c';code.write_text(r"""#include "dkrr_install.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static uid_t owner;static ino_t inode;static dev_t device;
static int fail_claim,fail_restore,fail_move,claims,restores,denials;
int __real_fstat(int,struct stat*);
int __wrap_fstat(int fd,struct stat *st){int rc=__real_fstat(fd,st);if(!rc&&st->st_ino==inode&&st->st_dev==device)st->st_uid=owner;return rc;}
int __real_fchown(int,uid_t,gid_t);
int __wrap_fchown(int fd,uid_t uid,gid_t gid){struct stat st;if(__real_fstat(fd,&st)==0&&st.st_ino==inode&&st.st_dev==device){
 if(uid==geteuid()){++claims;if(fail_claim){errno=EPERM;return -1;}}
 else{++restores;if(fail_restore){errno=EPERM;return -1;}}
 owner=uid;return 0;}return __real_fchown(fd,uid,gid);}
int __real_rename(const char*,const char*);
int __wrap_rename(const char*a,const char*b){struct stat st;if(lstat(a,&st)==0&&st.st_ino==inode&&st.st_dev==device){
 if(owner!=geteuid()&&!(st.st_mode&S_IWOTH)){++denials;errno=EACCES;return -1;}
 if(fail_move){errno=EIO;return -1;}}
 return __real_rename(a,b);}
static char from[1024],to[1024];
static void setup(const char *root,unsigned n,int app_owned){char base[1024],p[1024];int fd;struct stat st;
 snprintf(base,sizeof(base),"%s/case-%u",root,n);assert(mkdir(base,0700)==0);
 snprintf(p,sizeof(p),"%s/source",base);assert(mkdir(p,0700)==0);snprintf(from,sizeof(from),"%s/source/app",base);assert(mkdir(from,0755)==0);
 snprintf(p,sizeof(p),"%s/target",base);assert(mkdir(p,0755)==0);snprintf(to,sizeof(to),"%s/target/app",base);
 snprintf(p,sizeof(p),"%s/keep",from);fd=open(p,O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0&&write(fd,"unchanged",9)==9&&close(fd)==0);
 assert(lstat(from,&st)==0);inode=st.st_ino;device=st.st_dev;owner=geteuid()+(app_owned?1:0);fail_claim=fail_restore=fail_move=claims=restores=denials=0;}
static int exists(const char*p){struct stat st;return lstat(p,&st)==0;}
static void check_content(const char *dir){char p[1200],buf[16];struct stat st;snprintf(p,sizeof(p),"%s/keep",dir);int fd=open(p,O_RDONLY);assert(fd>=0&&read(fd,buf,sizeof(buf))==9&&!memcmp(buf,"unchanged",9)&&close(fd)==0);assert(lstat(dir,&st)==0&&(st.st_mode&07777)==0755);assert(lstat(p,&st)==0&&(st.st_mode&0777)==0600);}
int main(int argc,char**argv){assert(argc==2);uid_t app=geteuid()+1;
 setup(argv[1],0,1);assert(rename(from,to)<0&&errno==EACCES&&denials==1);assert(dkrr_move_app_directory(from,to)==0);assert(!exists(from)&&exists(to)&&owner==app&&claims==1&&restores==1);check_content(to);
 setup(argv[1],1,1);fail_claim=1;assert(dkrr_move_app_directory(from,to)<0&&errno==EPERM&&owner==app&&claims==1&&restores==0&&exists(from)&&!exists(to));check_content(from);
 setup(argv[1],2,1);fail_move=1;assert(dkrr_move_app_directory(from,to)<0&&errno==EIO&&owner==app&&claims==1&&restores==1&&exists(from)&&!exists(to));check_content(from);
 setup(argv[1],3,1);fail_restore=1;assert(dkrr_move_app_directory(from,to)<0&&errno==EPERM&&!exists(from)&&exists(to)&&owner==geteuid());check_content(to);fail_restore=0;assert(dkrr_move_app_directory(to,from)==0);check_content(from);
 setup(argv[1],4,0);assert(dkrr_move_app_directory(from,to)==0&&claims==0&&restores==0);check_content(to);
 setup(argv[1],5,1);assert(mkdir(to,0755)==0);assert(dkrr_move_app_directory(from,to)<0&&errno==EEXIST&&claims==0&&owner==app);check_content(from);
 setup(argv[1],6,1);char link[1100];snprintf(link,sizeof(link),"%s-link",from);assert(symlink(from,link)==0);assert(dkrr_move_app_directory(link,to)<0&&claims==0&&owner==app);check_content(from);
 puts("PASS: PS5 cross-parent EACCES reproduced; descriptor owner claim/restore, preserved modes/content, claim/move/restore faults, recovery, collision and symlink refusal");return 0;}
""")
    binary=work/'owner'
    subprocess.run(self.flags+['-Wall','-Wextra','-Werror','-I',str(ROOT/'installer'),str(code),str(self.base/'files.c.o'),str(self.base/'hash.c.o'),'-Wl,--wrap=fstat','-Wl,--wrap=fchown','-Wl,--wrap=rename','-o',str(binary)],check=True)
    subprocess.run([str(binary),str(work)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'),check=True)
 def test_network_disconnect_or_bad_commit_changes_nothing(self):
    for command in (b'',b'BADGO12\n'):
        p=self.network_process();self.assertFalse((self.data/'dkrr').exists())
        if command:p.stdin.write(command);p.stdin.flush()
        p.stdin.close();p.wait(timeout=5)
        self.assertEqual(p.returncode,10,p.stdout.read()+p.stderr.read())
        self.assertFalse((self.data/'dkrr').exists());p.stdout.close();p.stderr.close()
 def test_network_waits_for_game_then_keeps_saved_data(self):
    import time
    self.run_installer();before=(self.live/'eboot.bin').read_bytes()
    saved=self.data/'dkrr/profile-us-v77/saves';saved.mkdir();(saved/'keep.bin').write_bytes(b'keep network update progress')
    lease=open(self.data/'dkrr/profile-us-v77/.app-run.lock','rb');fcntl.flock(lease,fcntl.LOCK_SH)
    p=self.network_process();self.assertEqual((self.live/'eboot.bin').read_bytes(),before)
    p.stdin.write(b'DKRR');p.stdin.flush();time.sleep(.05);self.assertIsNone(p.poll())
    p.stdin.write(b'GO1\n');p.stdin.flush();p.stdin.close();time.sleep(.15)
    self.assertIsNone(p.poll());self.assertEqual((self.live/'eboot.bin').read_bytes(),before)
    fcntl.flock(lease,fcntl.LOCK_UN);lease.close();p.wait(timeout=20)
    self.assertEqual(p.returncode,0,p.stdout.read()+p.stderr.read());p.stdout.close();p.stderr.close()
    self.assertIn(b'3'*24,(self.live/'eboot.bin').read_bytes())
    self.assertEqual((saved/'keep.bin').read_bytes(),b'keep network update progress')
 def test_fresh_repeat_update_and_second_update(self):
    self.run_installer();saved=self.data/'dkrr/profile-us-v77/saves';saved.mkdir();(saved/'keep.bin').write_bytes(b'keep progress')
    config=self.data/'dkrr/profile-us-v77/general.json';config.write_bytes(b'keep settings')
    self.run_installer();self.run_installer(1);self.run_installer(2)
    self.assertEqual((saved/'keep.bin').read_bytes(),b'keep progress');self.assertEqual(config.read_bytes(),b'keep settings')
    self.assertEqual((self.data/'shadowmount/manual.lst').read_bytes(),b'# preserve notes\n/mnt/usb1/other-game\n'+str(self.live).encode()+b'\n')
    self.assertIn(b'3'*24,(self.live/'eboot.bin').read_bytes())
    self.assertEqual(len(list((self.data/'dkrr/installation/history').iterdir())),2)
    self.run_installer(0,False);self.assertIn(b'3'*24,(self.live/'eboot.bin').read_bytes())
    for rev in (77,80):
     self.assertEqual((self.data/f'dkrr/roms/dkr.us.v{rev}.z64').read_bytes(),self.roms[rev])
     self.assertEqual(len(list((self.live/f'assets/controller-audio/us-v{rev}').glob('*.pcm'))),32)
    # Updates use internal ROMs after USB has been removed.
    shutil.rmtree(self.mnt/'usb0');self.run_installer(2)
 def test_exact_legacy_migration_and_update_only_guard(self):
    self.run_installer(2,False);self.assertFalse(self.live.exists())
    self.live.parent.mkdir(parents=True);shutil.copytree(self.legacy,self.live)
    r=self.run_installer(0,False,DKRR_POWER_CUT='1');self.assertEqual(r.returncode,77)
    self.run_installer(0);self.assertIn(b'1'*24,(self.live/'eboot.bin').read_bytes())
 def test_native_update_directory_access_and_pending_transaction_guard(self):
    self.run_installer()
    for path in (self.live,self.live/'sce_sys',self.live/'sce_module'):
     self.assertEqual(path.stat().st_uid,os.getuid());self.assertEqual(path.stat().st_mode&0o777,0o755)
    pending=self.data/'dkrr/preferences/update-client-1/native-install';pending.mkdir(parents=True);(pending/'unknown').write_bytes(b'preserve pending recovery')
    before=(self.live/'eboot.bin').read_bytes();self.run_installer(1,False)
    self.assertEqual((self.live/'eboot.bin').read_bytes(),before);self.assertEqual((pending/'unknown').read_bytes(),b'preserve pending recovery')
 def test_wrong_missing_rom_and_symlink(self):
    (self.mnt/'usb0/DKRR/owned 80.z64').write_bytes(b'bad'.ljust(SIZE,b'\0'));self.run_installer(ok=False);self.assertFalse(self.live.exists())
    self.setUp();p=self.mnt/'usb0/DKRR/owned 80.z64';p.unlink();p.symlink_to(self.mnt/'usb0/DKRR/owned 77.z64');self.run_installer(ok=False);self.assertFalse(self.live.exists())
 def test_all_byte_orders(self):
    p=self.mnt/'usb0/DKRR/owned 77.z64';data=bytearray(self.roms[77]);data[0::2],data[1::2]=data[1::2],data[0::2];p.write_bytes(data)
    data=bytearray(self.roms[80]);data[0::4],data[1::4],data[2::4],data[3::4]=data[3::4],data[2::4],data[1::4],data[0::4];(self.mnt/'usb0/DKRR/owned 80.z64').write_bytes(data)
    self.run_installer();self.assertEqual((self.data/'dkrr/roms/dkr.us.v77.z64').read_bytes(),self.roms[77])
 def test_crowded_unrelated_usb_before_rom_drive(self):
    (self.mnt/'usb1').mkdir();(self.mnt/'usb0/DKRR').rename(self.mnt/'usb1/DKRR')
    for i in range(300):(self.mnt/f'usb0/unrelated-{i}.txt').write_bytes(b'keep')
    self.run_installer()
    self.assertEqual(len(list((self.mnt/'usb0').iterdir())),300)
    self.assertEqual((self.data/'dkrr/roms/dkr.us.v80.z64').read_bytes(),self.roms[80])
 def test_non_rom_size_match_checks_header_before_reading_body(self):
    (self.mnt/'usb1').mkdir();(self.mnt/'usb0/DKRR').rename(self.mnt/'usb1/DKRR')
    unrelated=self.mnt/'usb0/not-a-rom.bin'
    with unrelated.open('wb') as f:f.truncate(SIZE)
    self.run_installer(DKRR_REJECT_FULL_NONROM_READ='1')
    self.assertEqual(unrelated.stat().st_size,SIZE)
 def test_unknown_app_and_running_game(self):
    self.run_installer();original=(self.live/'eboot.bin').read_bytes();(self.live/'eboot.bin').write_bytes(b'foreign');self.run_installer(1,False)
    self.assertEqual((self.live/'eboot.bin').read_bytes(),b'foreign');(self.live/'eboot.bin').write_bytes(original)
    with (self.data/'dkrr/profile-us-v77/.app-run.lock').open('rb') as f:
     fcntl.flock(f,fcntl.LOCK_SH|fcntl.LOCK_NB);self.run_installer(1,False)
    self.assertEqual((self.live/'eboot.bin').read_bytes(),original)
 def test_failed_activation_resume_and_power_cut(self):
    self.run_installer();old=(self.live/'eboot.bin').read_bytes()
    self.run_installer(1,False,DKRR_FAIL_ACTIVATE='1');self.assertEqual((self.live/'eboot.bin').read_bytes(),old)
    self.run_installer(1);self.assertIn(b'2'*24,(self.live/'eboot.bin').read_bytes())
    r=self.run_installer(2,False,DKRR_POWER_CUT='1');self.assertEqual(r.returncode,77);self.assertFalse(self.live.exists())
    self.run_installer(2);self.assertIn(b'3'*24,(self.live/'eboot.bin').read_bytes())
 def test_partial_write_and_cut_after_activation(self):
    self.run_installer();old=(self.live/'eboot.bin').read_bytes()
    self.run_installer(1,False,DKRR_FAIL_WRITE='1');self.assertEqual((self.live/'eboot.bin').read_bytes(),old)
    self.run_installer(1)
    r=self.run_installer(2,False,DKRR_POWER_AFTER_ACTIVATE='1');self.assertEqual(r.returncode,78)
    self.assertIn(b'3'*24,(self.live/'eboot.bin').read_bytes());self.run_installer(2)
 def test_failed_backup_directory_sync_keeps_recovery_record(self):
    self.run_installer();old=(self.live/'eboot.bin').read_bytes()
    r=self.run_installer(1,False,DKRR_FAIL_HISTORY_SYNC='1');self.assertEqual(r.returncode,7)
    self.assertFalse(self.live.exists())
    self.assertTrue((self.data/'dkrr/installation/transaction.manifest').is_file())
    history=list((self.data/'dkrr/installation/history').iterdir());self.assertEqual(len(history),1)
    self.assertEqual((history[0]/'eboot.bin').read_bytes(),old)
    self.run_installer(1);self.assertIn(b'2'*24,(self.live/'eboot.bin').read_bytes())
    self.assertFalse((self.data/'dkrr/installation/transaction.manifest').exists())
 def test_corrupt_managed_manifest_is_refused(self):
    self.run_installer();p=self.live/'dkrr-install.manifest';p.write_text('DKRR-INSTALL-2\ninvalid\n1\n../outside\n')
    old=(self.live/'eboot.bin').read_bytes();self.run_installer(1,False);self.assertEqual((self.live/'eboot.bin').read_bytes(),old)
 def test_foreign_registered_title_identity_is_protected(self):
    p=self.user/'app/PPSA94678/sce_sys/param.json';p.parent.mkdir(parents=True);p.write_bytes(b'unrelated registered title')
    self.run_installer(0,False);self.assertFalse(self.live.exists());self.assertEqual(p.read_bytes(),b'unrelated registered title')
    p.write_bytes((self.base/'app0/sce_sys/param.json').read_bytes());self.run_installer()
 def test_registration_preserves_unknown_link_and_resumes(self):
    path=self.data/'shadowmount/manual.lst';outside=self.base/'unrelated-list.txt';outside.write_bytes(b'keep unrelated data')
    path.unlink();path.symlink_to(outside)
    r=self.run_installer(0,False);self.assertEqual(r.returncode,9);self.assertEqual(outside.read_bytes(),b'keep unrelated data')
    installed=(self.live/'eboot.bin').read_bytes();path.unlink();path.write_text('# retained\n')
    self.run_installer();self.assertEqual((self.live/'eboot.bin').read_bytes(),installed)
    self.assertEqual(path.read_bytes(),b'# retained\n'+str(self.live).encode()+b'\n')
 def test_registration_preserves_stale_and_foreign_temporary_files(self):
    folder=self.data/'shadowmount';path=folder/'manual.lst';before=path.read_bytes()
    legacy=folder/'manual.lst.dkrr.tmp';legacy.write_bytes(b'old interrupted registration snapshot')
    outside=self.base/'unrelated-registration.txt';outside.write_bytes(b'keep unrelated registration')
    link=folder/'manual.lst.dkrr.0.tmp';link.symlink_to(outside)
    partial=folder/'manual.lst.dkrr.1.tmp';partial.write_bytes(b'# previous list before another app changed it\n')
    self.run_installer()
    self.assertEqual(path.read_bytes(),before+str(self.live).encode()+b'\n')
    self.assertEqual(legacy.read_bytes(),b'old interrupted registration snapshot')
    self.assertTrue(link.is_symlink());self.assertEqual(outside.read_bytes(),b'keep unrelated registration')
    self.assertEqual(partial.read_bytes(),b'# previous list before another app changed it\n')
 def test_registration_slot_exhaustion_preserves_existing_list(self):
    folder=self.data/'shadowmount';path=folder/'manual.lst';before=path.read_bytes()
    for slot in range(128):(folder/f'manual.lst.dkrr.{slot}.tmp').write_bytes(b'preserve occupied slot')
    result=self.run_installer(0,False);self.assertEqual(result.returncode,9)
    self.assertEqual(path.read_bytes(),before)
    for slot in range(128):self.assertEqual((folder/f'manual.lst.dkrr.{slot}.tmp').read_bytes(),b'preserve occupied slot')
 def test_package_positive_inventory_refuses_extra_user_data(self):
    app=self.base/'app0';p=app/'assets/personal.json';p.write_text('synthetic unrelated user data')
    try:
     with self.assertRaises(ValueError):release_files(app)
    finally:p.unlink()
 def test_staging_exact_prefix_and_unknown_bytes(self):
    self.run_installer();stage=self.data/('dkrr/installation/staged-integrated-'+'2'*24+'-r2');stage.mkdir()
    (stage/'.dkrr-stage-id').write_text('integrated-'+'2'*24+'-r2')
    (stage/'eboot.bin').write_bytes(bytes.fromhex('4f153d1d'))
    self.run_installer(1)
    stage=self.data/('dkrr/installation/staged-integrated-'+'3'*24+'-r3');stage.mkdir();(stage/'.dkrr-stage-id').write_text('integrated-'+'3'*24+'-r3')
    (stage/'eboot.bin').write_bytes(b'unknown');before=(self.live/'eboot.bin').read_bytes();self.run_installer(2,False)
    self.assertEqual((stage/'eboot.bin').read_bytes(),b'unknown');self.assertEqual((self.live/'eboot.bin').read_bytes(),before)
if __name__=='__main__':unittest.main()
