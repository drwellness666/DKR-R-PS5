/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ps5_net.h"
#include "pollinterrupter.hpp"
#include <juice/juice.h>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <fstream>
#include <atomic>
#include <chrono>
#include <cstring>
extern "C" void dkrr_test_net_fail(int);
static std::atomic_uint fatal_messages;
static void log_message(juice_log_level_t level,const char*message){if(level==JUICE_LOG_LEVEL_FATAL){++fatal_messages;fprintf(stderr,"RTC fatal: %s\n",message);}}
struct Peer {juice_agent_t *agent=nullptr;Peer *other=nullptr;std::atomic_uint received{0};};
static void candidate(juice_agent_t*,const char*value,void*opaque){auto *p=static_cast<Peer*>(opaque);assert(p->other&&p->other->agent);assert(!juice_add_remote_candidate(p->other->agent,value));}
static void receive(juice_agent_t*,const char*value,size_t bytes,void*opaque){assert(bytes==4&&!std::memcmp(value,"DKRR",4));++static_cast<Peer*>(opaque)->received;}
static void balanced(){auto n=dkrr_net_counters_read();if(n.opened!=n.closed||n.epolls_opened!=n.epolls_closed||n.close_errors)fprintf(stderr,"unbalanced opened=%llu closed=%llu errors=%llu epoll=%llu/%llu\n",(unsigned long long)n.opened,(unsigned long long)n.closed,(unsigned long long)n.close_errors,(unsigned long long)n.epolls_opened,(unsigned long long)n.epolls_closed);assert(n.opened==n.closed&&n.epolls_opened==n.epolls_closed&&!n.close_errors);}
int main(){
 alarm(25);
 for(int fail=1;fail<=9;++fail){
  dkrr_test_net_fail(fail);bool threw=false;
  try {rtc::impl::PollInterrupter wake;}catch(const std::runtime_error&){threw=true;}
  assert(threw);balanced();
 }
 dkrr_test_net_fail(0);
 {
  rtc::impl::PollInterrupter wake;pollfd pfd{};wake.prepare(pfd);
  for(int round=0;round<10;++round){
   for(int i=0;i<4000;++i)wake.interrupt();
   assert(dkrr_net_poll(&pfd,1,0)==1);wake.process(pfd);assert(dkrr_net_poll(&pfd,1,0)==0);
  }
  std::thread producer([&]{for(int i=0;i<20000;++i)wake.interrupt();});
  for(int i=0;i<200;++i){dkrr_net_poll(&pfd,1,1);wake.process(pfd);}
  producer.join();dkrr_net_poll(&pfd,1,0);wake.process(pfd);
 }
 balanced();
 juice_set_log_level(JUICE_LOG_LEVEL_FATAL);
 juice_set_log_handler(log_message);
 for(int i=0;i<32;++i){juice_config_t config{};config.concurrency_mode=JUICE_CONCURRENCY_MODE_POLL;
  auto *agent=juice_create(&config);assert(agent);assert(!juice_gather_candidates(agent));
  std::this_thread::sleep_for(std::chrono::milliseconds(2));juice_destroy(agent);}
 balanced();
 assert(!fatal_messages.load());
 {
  Peer a,b;a.other=&b;b.other=&a;juice_config_t config{};config.concurrency_mode=JUICE_CONCURRENCY_MODE_POLL;config.cb_candidate=candidate;config.cb_recv=receive;
  config.user_ptr=&a;a.agent=juice_create(&config);config.user_ptr=&b;b.agent=juice_create(&config);assert(a.agent&&b.agent);
  char local_a[JUICE_MAX_SDP_STRING_LEN],local_b[JUICE_MAX_SDP_STRING_LEN];
  assert(!juice_get_local_description(a.agent,local_a,sizeof(local_a))&&!juice_get_local_description(b.agent,local_b,sizeof(local_b)));
  assert(!juice_set_remote_description(a.agent,local_b)&&!juice_set_remote_description(b.agent,local_a));
  assert(!juice_gather_candidates(a.agent)&&!juice_gather_candidates(b.agent));
  auto connected=[](juice_agent_t*agent){auto s=juice_get_state(agent);return s==JUICE_STATE_CONNECTED||s==JUICE_STATE_COMPLETED;};
  for(int i=0;i<500&&(!connected(a.agent)||!connected(b.agent));++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
  assert(connected(a.agent)&&connected(b.agent));assert(!juice_send(a.agent,"DKRR",4)&&!juice_send(b.agent,"DKRR",4));
  for(int i=0;i<100&&(!a.received||!b.received);++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
  assert(a.received&&b.received);juice_destroy(a.agent);juice_destroy(b.agent);
 }
 balanced();assert(!fatal_messages.load());
 // C++ stream.close must keep its ordinary file semantics under the aliases.
 std::fstream file;file.close();
 puts("PASS actual RTC on native IDs: failure cleanups, saturation, concurrent drain, 32 registry lifetimes, two real ICE peers connected with bidirectional data, no internal Fatal errors or socket/epoll leaks");
}
