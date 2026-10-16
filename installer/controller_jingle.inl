/* SPDX-License-Identifier: GPL-3.0-or-later
 * Bounded CSeq note parsing and mono sample-bank rendering for owned jingles.
 * No sequence, instrument or sample bytes are stored here. Unsupported event
 * forms are refused; this is not a replacement for the game's full mixer.
 */
struct jtrack {const unsigned char *data;size_t size,pos,back,left;int ok;};
static unsigned jb(struct jtrack*t){
 if(!t->ok)return 0;
 if(t->left){if(t->back>=t->size){t->ok=0;return 0;}--t->left;return t->data[t->back++];}
 if(t->pos>=t->size){t->ok=0;return 0;}unsigned b=t->data[t->pos++];
 if(b==254){if(t->pos>=t->size){t->ok=0;return 0;}unsigned hi=t->data[t->pos++];
  if(hi!=254){if(t->size-t->pos<2){t->ok=0;return 0;}unsigned lo=t->data[t->pos++],n=t->data[t->pos++];size_t distance=(hi<<8)|lo;
   if(!n||distance+4>t->pos){t->ok=0;return 0;}t->back=t->pos-distance-4;t->left=n;
   if(t->back>=t->size||n>t->size-t->back){t->ok=0;return 0;}--t->left;b=t->data[t->back++];}}
 return b;
}
static unsigned jv(struct jtrack*t){unsigned n=0;for(unsigned i=0;i<4;++i){unsigned b=jb(t);n=(n<<7)|(b&127);if(!(b&128))return n;}t->ok=0;return 0;}
struct jevent {unsigned tick,order,kind,ch,a,b,duration;};
static int jcompare(const void*a,const void*b){const struct jevent*x=a,*y=b;return x->tick<y->tick?-1:x->tick>y->tick?1:x->order<y->order?-1:x->order>y->order;}
static int jparse(const unsigned char*data,size_t size,struct jevent events[256],unsigned*count,unsigned*division){
 struct reader r={data,size,1};if(size<68||size>16384)return -1;*division=u32(&r,64);*count=0;
 if(!*division||*division>960)return -1;
 for(unsigned i=0;i<16;++i){unsigned pos=u32(&r,4*i);if(!pos)continue;if(pos<68||pos>=size)return -1;
  struct jtrack t={data,size,pos,0,0,1};unsigned tick=0,status=0;int ended=0;
  for(unsigned budget=0;budget<256;++budget){unsigned delta=jv(&t);if(delta>100000||tick>100000-delta)return -1;tick+=delta;
   unsigned v=jb(&t);struct jevent e={tick,*count,0,0,0,0,0};
   if(v==255){unsigned kind=jb(&t);status=0;if(kind==47){ended=1;break;}
    if(kind!=81)return -1;e.kind=0x51;e.a=(jb(&t)<<16);e.a|=jb(&t)<<8;e.a|=jb(&t);
    if(tick||e.a<20000||e.a>5000000)return -1;
   }else{if(v&128){status=v;e.a=jb(&t);}else e.a=v;
    e.kind=status>>4;e.ch=status&15;if(e.kind<8||e.kind>14||e.a>127)return -1;
    if(e.kind!=12&&e.kind!=13)e.b=jb(&t);if(e.b>127)return -1;
    if(e.kind==9)e.duration=jv(&t);
   }
   if(!t.ok||*count>=256)return -1;events[(*count)++]=e;
  }if(!ended||!t.ok)return -1;
 }if(!r.ok||!*count)return -1;qsort(events,*count,sizeof(*events),jcompare);return 0;
}
static int jnote(struct reader*r,uint64_t bank,uint64_t samples,uint64_t instrument,unsigned rate,unsigned note,unsigned velocity,double gain,double seconds,double gate,int64_t*mix,size_t*used){
 unsigned sounds=u16(r,instrument+14);if(!sounds||sounds>256)return -1;uint64_t sound=0;
 for(unsigned i=0;i<sounds;++i){uint64_t s=bank+u32(r,instrument+16+4*i),key=bank+u32(r,s+4);
  if(velocity>=byte(r,key)&&velocity<=byte(r,key+1)&&note>=byte(r,key+2)&&note<=byte(r,key+3)){sound=s;break;}}
 if(!r->ok||!sound)return -1;uint64_t key=bank+u32(r,sound+4),wave=bank+u32(r,sound+8),env=bank+u32(r,sound);
 uint64_t base=samples+u32(r,wave);unsigned length=u32(r,wave+4),kind=byte(r,wave+8);if(!r->ok||!length||length>2097152)return -1;
 size_t frames=kind==0?(length/9)*16:length/2;if(!frames)return -1;int16_t*pcm=malloc(frames*2);if(!pcm)return -1;
 if(kind==0){uint64_t book=bank+u32(r,wave+16);unsigned order=u32(r,book),predictors=u32(r,book+4);int16_t coefficients[256];
  if(order!=2||!predictors||predictors>16||length%9>1){free(pcm);return -1;}
  for(unsigned i=0;i<order*predictors*8;++i)coefficients[i]=(int16_t)u16(r,book+8+2*i);
  const unsigned char*data=read_at(r,base,length-length%9);
  if(!r->ok||!data||dkrr_vadpcm(data,length-length%9,order,predictors,coefficients,pcm)<0){free(pcm);return -1;}
 }else if(kind==1&&!(length%2)){for(size_t i=0;i<frames;++i)pcm[i]=(int16_t)u16(r,base+2*i);}
 else{free(pcm);return -1;}
 unsigned loop_start=0,loop_end=0;uint64_t loop=u32(r,wave+12);
 if(loop){loop+=bank;loop_start=u32(r,loop);loop_end=u32(r,loop+4);if(loop_start>=loop_end||loop_end>frames){free(pcm);return -1;}}
 double speed=pow(2.0,((int)note*100-(int)byte(r,key+4)*100-(int8_t)byte(r,key+5))/1200.0);
 int32_t attack=(int32_t)u32(r,env),decay=(int32_t)u32(r,env+4),release=(int32_t)u32(r,env+8);
 unsigned av=byte(r,env+12),dv=byte(r,env+13);
 if(!r->ok||rate<8000||rate>96000||speed<0.01||speed>64||seconds<0||seconds>1.5||gate<0||gate>4){free(pcm);return -1;}
 gain*=byte(r,instrument)/127.0*byte(r,sound+13)/127.0*(double)velocity/127.0;
 double rel=min(0.3,max(0.005,release/1000000.0));size_t start=(size_t)(seconds*48000),total=(size_t)(min(1.5-seconds,gate+rel)*48000);
 for(size_t i=0;i<total;++i){double pos=(double)i*rate*speed/48000;
  if(loop&&pos>=loop_end)pos=loop_start+fmod(pos-loop_start,loop_end-loop_start);
  size_t j=(size_t)pos;if(j>=frames)break;double f=pos-j,sample=pcm[j]*(1-f)+pcm[j+1<frames?j+1:j]*f;
  double time=(double)i/48000,us=time*1000000,envelope;
  if(attack>0&&us<attack)envelope=av/127.0*us/attack;
  else if(decay>0)envelope=(av+((int)dv-(int)av)*min(1,max(0,us-max(0,attack))/decay))/127;
  else envelope=av/127.0;
  if(time>gate)envelope*=max(0,1-(time-gate)/rel);
  double fade=min(1,(double)i/96);mix[start+i]+=(int64_t)nearbyint(sample*gain*envelope*fade);
  if(*used<start+i+1)*used=start+i+1;
 }free(pcm);return 0;
}
static int jingle(struct reader*r,uint64_t audio,uint64_t table,unsigned sequence,const char*dest,unsigned slot){
 uint64_t seqbase=audio+u32(r,table+16);if(u16(r,seqbase)!=0x5331||sequence>=u16(r,seqbase+2))return -1;
 unsigned offset=u32(r,seqbase+4+sequence*8),size=u32(r,seqbase+8+sequence*8);
 const unsigned char*data=read_at(r,seqbase+offset,size);struct jevent events[256];unsigned count,division;
 if(!r->ok||!data||jparse(data,size,events,&count,&division)<0)return -1;
 uint64_t bank=audio+u32(r,audio+4),samples=audio+u32(r,table);if(u16(r,audio)!=0x4231)return -1;
 unsigned rate=u32(r,bank+4),program[16]={0},volume[16],expression[16];for(unsigned c=0;c<16;++c)volume[c]=expression[c]=127;
 unsigned tempo=500000;size_t used=0;int64_t*mix=calloc(72000,sizeof(*mix));if(!mix)return -1;int result=-1;
 for(unsigned i=0;i<count;++i){struct jevent*e=&events[i];if(e->kind==0x51){tempo=e->a;continue;}
  if(e->kind==12){program[e->ch]=e->a;continue;}
  if(e->kind==11){if(e->a==7)volume[e->ch]=e->b;if(e->a==11)expression[e->ch]=e->b;continue;}
  if(e->kind!=9||!e->b)continue;if(program[e->ch]>=u16(r,bank))goto done;
  uint64_t instrument=audio+u32(r,bank+12+program[e->ch]*4);
  if(jnote(r,audio,samples,instrument,rate,e->a,e->b,(double)volume[e->ch]*expression[e->ch]/(127.0*127.0),
      (double)e->tick*tempo/(division*1000000.0),(double)e->duration*tempo/(division*1000000.0),mix,&used)<0)goto done;
 }
 if(!r->ok||!used)goto done;int64_t peak=0;for(size_t i=0;i<used;++i){int64_t n=mix[i]<0?-mix[i]:mix[i];if(n>peak)peak=n;}if(!peak)goto done;
 int16_t*out=malloc(used*2);if(!out)goto done;double gain=min(1,30000.0/peak);
 for(size_t i=0;i<used;++i)out[i]=(int16_t)nearbyint(mix[i]*gain*min(1,(double)(used-1-i)/240));
 char path[DKRR_PATH];if(snprintf(path,sizeof(path),"%s/%u.pcm",dest,slot)>=(int)sizeof(path)){free(out);goto done;}
 result=dkrr_write_new(path,out,used*2,0644);free(out);
 done:free(mix);return result;
}
