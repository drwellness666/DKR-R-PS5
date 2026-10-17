// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_installation_lease.hpp"
#include <cassert>
#include <cstdio>
int main(){
 int game=dkrr_runtime_installation_lease();assert(game>=0);
 int installer=open(DKRR_GAME_LEASE_PATH,O_RDWR|O_NOFOLLOW);assert(installer>=0);
 assert(flock(installer,LOCK_EX|LOCK_NB)<0);close(game);
 assert(flock(installer,LOCK_EX|LOCK_NB)==0);
 assert(dkrr_runtime_installation_lease()<0);close(installer);
 game=dkrr_runtime_installation_lease();assert(game>=0);
 assert(flock(game,LOCK_EX|LOCK_NB)==0);
 assert(dkrr_runtime_installation_lease()<0);
 assert(flock(game,LOCK_SH|LOCK_NB)==0);close(game);
 unlink(DKRR_GAME_LEASE_PATH);std::puts("PASS native game/installer exclusion in both directions");
}
