// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_item_feedback.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
int main(){
 using namespace dkrr::ps5_items;
 Item held{0x80401100,1,2,true},spent=held;spent.quantity=1;
 std::vector<unsigned> commands;
 auto send=[&](const Command& c){assert(c.magic==0x44545247&&c.strength<=5&&!c.reserved[0]&&!c.reserved[1]);commands.push_back(c.mode);return 0;};
 Output out;
 assert(out.update(false,held,100,false,send)==0);
 assert(out.update(true,held,100,false,send)==0); // Merely holding an item does nothing.
 assert(out.update(true,spent,108,true,send)==1&&commands.back()==2); // First pull can fire immediately.
 for(unsigned t=109;t<178;++t)assert(out.update(true,spent,t,true,send)==0);
 assert(out.update(true,spent,178,true,send)==1&&commands.back()==1);
 assert(out.update(true,spent,180,false,send)==1&&commands.back()==0);
 assert(out.update(true,spent,181,false,send)==0);
 assert(out.update(true,spent,190,true,send)==1&&commands.back()==1);
 assert(out.update(true,{},191,true,send)==1&&commands.back()==0); // Pause / load / stale / disconnect.
 assert(out.update(true,spent,200,true,send)==1&&commands.back()==1);
 assert(out.update(false,spent,201,true,send)==1&&commands.back()==0);
 Item shield=held;shield.type=3;shield.quantity=1;assert(out.update(true,shield,202,true,send)==0);
 Item other=spent;other.racer+=4096;assert(out.update(true,other,203,true,send)==1&&commands.back()==1); // New racer does not cause a shot.
 Output failure;unsigned calls=0;
 auto fail=[&](const Command&){++calls;return -1;};
 assert(failure.update(true,held,300,true,fail)==-1);
 for(unsigned t=301;t<310;++t)assert(failure.update(true,held,t,true,fail)==0);
 assert(calls==1);assert(failure.update(false,{},311,false,send)==1&&commands.back()==0);
 assert(failure.update(true,held,312,true,send)==1&&commands.back()==1);
 assert(failure.update(false,{},313,false,send)==1&&commands.back()==0);
 Output retry;calls=0;
 assert(retry.update(true,held,320,true,fail)==-1&&calls==1);
 assert(retry.update(false,{},321,false,fail)==0&&calls==2);
 for(unsigned t=322;t<922;++t)assert(retry.update(false,{},t,false,fail)==0);
 assert(calls==2);
 assert(retry.update(true,held,923,true,send)==1&&commands.back()==1);
 assert(retry.update(false,{},924,false,send)==1&&commands.back()==0);
 Output release;
 assert(release.update(true,held,400,true,send)==1);
 assert(release.update(true,spent,408,false,send)==1&&commands.back()==2);
 assert(release.update(true,spent,479,false,send)==1&&commands.back()==0);
 for(unsigned type=0;type<5;++type){
  Output each;Item a=held;a.type=type;Item b=a;b.quantity=0;
  each.update(true,a,600,true,send);
  assert(each.update(true,b,620,false,send)==1&&commands.back()==2);
  assert(each.update(true,b,691,false,send)==1&&commands.back()==0);
 }
 Mailbox box;std::array<Item,4> items{};items[0]=held;items[1]=other;
 box.publish(items,0xFFFFF0);assert(box.read(0,0x1000010,false).quantity==2);
 assert(!box.read(0,0x1000200,false).valid&&!box.read(4,0x1000010,false).valid&&!box.read(0,0x1000010,true).valid);
 puts("PASS item feedback: consumption edge, short pulse, resistance, per-racer reset, release/pause/disable cleanup, deduplicated bounded failures, explicit-toggle recovery after failed reset, and mailbox expiry/wrap");
}
