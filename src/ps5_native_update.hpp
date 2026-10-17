// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_internet_update.hpp"
namespace dkrr::updates::native {
// Prepare only while the game holds its shared installation lease.
bool prepare(int package,const Offer&,char* error,std::size_t capacity);
// Called at startup, before game initialization. 1 means restored: close/restart.
int recover(int installation_lease,char* error,std::size_t capacity);
// Called after all game workers have joined, before releasing the same lease.
bool apply(int installation_lease,char* error,std::size_t capacity);
}
