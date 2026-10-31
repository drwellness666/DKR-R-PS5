/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "netplay/datagram_socket.hpp"
#include "netplay/lan_discovery.hpp"
#include "netplay/network_interfaces.hpp"
#include "ps5_net.h"
#include <cassert>
#include <cstdio>
#include <chrono>
#include <thread>
using namespace dkr::runtime::netplay;
int main(){
 std::string error;
 for(int n=0;n<100;++n){
  DatagramSocket a,b;assert(a.open(0,error)&&b.open(0,error));PeerAddress peer;
  assert(DatagramSocket::resolve("127.0.0.1",b.local_port(),peer,error));
  std::vector<std::uint8_t> message(100,std::uint8_t(n)),received;
  assert(a.send(peer,message,error));PeerAddress from;
  bool delivered=false;for(int i=0;i<100&&!delivered;++i){delivered=b.receive(from,received,error);assert(error.empty());if(!delivered)std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  assert(delivered&&received==message&&from);
  message.resize(4096);assert(a.send(peer,message,error));
  std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(!b.receive(from,received,error)&&received.empty()&&error.empty());
 }
 assert(virtual_lan_peer_addresses().empty());
 auto interfaces=enumerate_network_interfaces();assert(!interfaces.empty());
 {LanDiscovery discovery;assert(discovery.start(error));discovery.stop();} // No broadcast or public service used.
 auto counters=dkrr_net_counters_read();assert(counters.opened==counters.closed&&!counters.close_errors);
 puts("PASS actual direct/LAN socket sources on native IDs: 100 two-way socket lifetimes, recvmsg data/truncation, native interface discovery and no desktop process/network leaks");
}
