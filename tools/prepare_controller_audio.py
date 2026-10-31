#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare private item PCM from verified owned ROMs. Never distribute its output."""
import re,argparse,hashlib,json,math,pathlib,struct,shutil,subprocess,tempfile
from vadpcm_decode import decode
KNOWN={77:('0cb115d8716dbbc2922fda38e533b9fe63bb9670',969568),80:('6d96743d46f8c0cd0edb0ec5600b003c89b93755',970976)}
CUE_COUNT=int(re.search(r"^#define DKRR_CONTROLLER_CUE_COUNT ([0-9]+)$",(pathlib.Path(__file__).resolve().parents[1]/"src/ps5_controller_audio_bank.h").read_text(),re.M)[1])
# Actual item-use effects from the pinned decomp racer/audio paths.
SOUNDS=(0x21,0x232,0x233,0x135,0x134,0x151,0x0f,0x9f,0x13a,*range(0x156,0x160),0xa0,0xa1,0xa2,0x23d,0x107)
class Bank:
    def __init__(self,rom,base):
        self.data=rom
        count=self.u32(base)
        if count!=50:raise ValueError('Unexpected DKR main asset table')
        start=base+((count*4+4+15)//16)*16
        section=lambda i:start+self.u32(base+4*(i+1))
        self.audio=section(39);table=section(38)
        self.bank=self.audio+self.u32(table+4)
        self.samples=self.audio+self.u32(table+8)
        self.sfx=self.audio+self.u32(table+6*4)
        self.sfx_end=self.audio+self.u32(table+7*4)
        if self.u16(self.bank)!=0x4231:raise ValueError('Invalid DKR sound bank')
        bank=self.bank+self.u32(self.bank+4)
        self.rate=self.u32(bank+4)
        instrument=self.bank+self.u32(bank+12)
        self.sound_count=self.u16(instrument+14)
        self.instrument=instrument
    def read(self,offset,n):
        if offset<0 or n<0 or offset+n>len(self.data):raise ValueError('Truncated audio data')
        return self.data[offset:offset+n]
    def u16(self,p):return struct.unpack('>H',self.read(p,2))[0]
    def u32(self,p):return struct.unpack('>I',self.read(p,4))[0]
    def sound(self,sound_id):
        if self.sfx+10*(sound_id+1)>self.sfx_end:raise ValueError('Invalid sound effect id')
        index=self.u16(self.sfx+10*sound_id)
        gain=self.read(self.sfx+10*sound_id+2,1)[0]/128
        pitch=self.read(self.sfx+10*sound_id+4,1)[0]/100
        mixed=[];seen=set();loop_header=None
        while index:
            if index in seen or len(seen)>=16 or not 1<=index<=self.sound_count:raise ValueError('Invalid sound sequence')
            seen.add(index)
            sound=self.bank+self.u32(self.instrument+16+4*(index-1))
            env=self.bank+self.u32(sound);key=self.bank+self.u32(sound+4);wave=self.bank+self.u32(sound+8)
            base=self.samples+self.u32(wave);length=self.u32(wave+4);kind=self.read(wave+8,1)[0]
            if length>2*1024*1024:raise ValueError('Unbounded effect sample')
            if kind==0:
                book=self.bank+self.u32(wave+16);order=self.u32(book);predictors=self.u32(book+4)
                if order!=2 or not 1<=predictors<=16:raise ValueError('Invalid predictor dimensions')
                coefficients=struct.unpack('>'+str(order*predictors*8)+'h',self.read(book+8,order*predictors*16))
                if length%9 not in (0,1):raise ValueError('Invalid aligned ADPCM sample length')
                pcm=decode(self.read(base,length-length%9),order,predictors,coefficients)
            elif kind==1:
                pcm=list(struct.unpack('>'+str(length//2)+'h',self.read(base,length)))
            else:raise ValueError('Unknown effect encoding')
            key_data=self.read(key,6);detune=struct.unpack('b',key_data[5:6])[0]
            speed=pitch*2**((key_data[4]*100-6000+detune)/1200)
            if not 0.1<=speed<=10 or not 8000<=self.rate<=96000:raise ValueError('Invalid effect sample rate')
            attack,decay,release=struct.unpack('>iii',self.read(env,12))
            attack_vol,decay_vol=self.read(env+12,2)
            volume=self.read(sound+13,1)[0]/127*gain
            # Keep an original, bounded activation cue for long looped effects.
            frames=min(48000*3,int(len(pcm)*48000/(self.rate*speed)))
            if sound_id==0x9f:
                loop=self.bank+self.u32(wave+12);begin,end,repeats=struct.unpack('>III',self.read(loop,12))
                if len(seen)!=1 or key_data[0] or key_data[2]&0xc0 or not begin<end<=len(pcm) or not repeats or not 0<release<=2_000_000:raise ValueError('Invalid original shield loop')
                loop_start=int(begin*48000/(self.rate*speed));frames=int(end*48000/(self.rate*speed))
                loop_header=struct.pack('<6I',0x4c524b44,1,loop_start,frames,int(release*48000/1_000_000),0)
            delay=min(48000,int(key_data[1]*33333*48000/1_000_000))
            if len(mixed)<delay+frames:mixed.extend([0]*(delay+frames-len(mixed)))
            for i in range(frames):
                pos=i*self.rate*speed/48000;j=int(pos);fraction=pos-j
                sample=pcm[j]*(1-fraction)+pcm[min(j+1,len(pcm)-1)]*fraction
                us=i*1_000_000/48000
                if attack>0 and us<attack:envelope=attack_vol/127*us/attack
                elif decay>0:envelope=(attack_vol+(decay_vol-attack_vol)*min(1,max(0,us-max(0,attack))/decay))/127
                else:envelope=attack_vol/127
                fade=1 if loop_header is not None else min(1,i/96,(frames-1-i)/240)
                mixed[delay+i]+=round(sample*volume*envelope*max(0,fade))
            index=key_data[0]+(key_data[2]&0xc0)*4
        peak=max((abs(x) for x in mixed),default=0)
        gain=min(1.0,30000/peak) if peak else 1.0
        return ([] if loop_header is None else list(struct.unpack("<12h",loop_header)))+[round(x*gain) for x in mixed]
def prepare(roms,destination):
    root=pathlib.Path(__file__).resolve().parents[1]
    selected=json.loads((roms/'selected-revisions.json').read_text())['revisions']
    if not selected or len(set(selected))!=len(selected) or any(rev not in KNOWN for rev in selected):raise ValueError('Invalid owned revision selection')
    compiler=shutil.which('clang') or shutil.which('cc')
    if not compiler:raise ValueError('The native audio preparer requires a host C compiler')
    receipt={}
    with tempfile.TemporaryDirectory(prefix='dkrr-owned-audio-') as temporary:
        binary=pathlib.Path(temporary)/'decode'
        subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror',str(root/'tools/controller_audio_host.c'),str(root/'installer/audio.c'),str(root/'installer/vadpcm.c'),'-lm','-o',str(binary)],check=True)
        for revision in sorted(selected):
            sha,_=KNOWN[revision];p=roms/f'dkr.us.v{revision}.z64'
            if p.is_symlink() or p.stat().st_size!=12*1024*1024:raise ValueError('Missing ordinary owned canonical ROM')
            if hashlib.sha1(p.read_bytes()).hexdigest()!=sha:raise ValueError('Owned ROM checksum mismatch')
            result=subprocess.run([str(binary),str(p),str(revision)],check=True,stdout=subprocess.PIPE)
            data=result.stdout
            if len(data)>CUE_COUNT*(384000+8):raise ValueError('Unbounded private PCM output')
            files={};at=0
            while at<len(data):
                if len(data)-at<8:raise ValueError('Truncated cue record')
                index,size=struct.unpack_from('<II',data,at);at+=8
                if index>=CUE_COUNT or index in files or not size or size%2 or size>384000 or size>len(data)-at:raise ValueError('Invalid cue record')
                files[index]=data[at:at+size];at+=size
            if set(files)!=set(range(CUE_COUNT)):raise ValueError('Incomplete private cue bank')
            folder=destination/f'us-v{revision}'
            if folder.is_symlink():raise ValueError('Private audio destination is a symlink')
            folder.mkdir(parents=True,exist_ok=True);rows=[]
            for index,blob in sorted(files.items()):
                target=folder/f'{index}.pcm'
                if target.is_symlink() or (target.exists() and target.read_bytes()!=blob):raise ValueError('Different private effect output already exists')
                if not target.exists():target.write_bytes(blob)
                rows.append(dict(index=index,**({'sound_id':SOUNDS[index]} if index<24 else {'sequence_id':43+index-24} if index<32 else {'sound_id':0x147}),frames=len(blob)//2,sha256=hashlib.sha256(blob).hexdigest()))
            receipt[str(revision)]=rows
    return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('roms',type=pathlib.Path);p.add_argument('destination',type=pathlib.Path);a=p.parse_args()
    print(json.dumps(prepare(a.roms,a.destination),indent=2));print('Private owned PCM prepared; never upload or share it.')
