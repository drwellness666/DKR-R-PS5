// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_https.hpp"
#include <cassert>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <chrono>
int main(int argc,char**argv){
    assert(argc==2);using namespace dkrr::https;const std::string origin=argv[1];
    auto get=[&](std::string route,std::uint64_t limit=128){Request r;r.url=origin+route;r.limit=limit;std::string data;auto response=perform(r,[&](auto p,auto n,const auto&){data.append(p,n);});return std::pair{response,data};};
    auto good=get("/good");assert(good.first.status==200&&good.second=="verified");
    auto redirect=get("/redirect");assert(redirect.first.status==302&&redirect.second=="redirect"); // caller owns follow policy
    auto missing=get("/missing");assert(missing.first.status==404);
    auto rejects=[&](Request r){bool failed=false;try{perform(r,{});}catch(const std::exception&){failed=true;}assert(failed);};
    Request r;r.url=origin+"/large";r.limit=16;rejects(r);
    r.url=origin+"/good";r.cancelled=[] {return true;};rejects(r);r.cancelled={};
    r.headers={{"Authorization","secret\r\nInjected: true"}};rejects(r);r.headers.clear();
    r.url=origin+"/good";r.url.replace(r.url.find("localhost"),9,"127.0.0.1");rejects(r); // hostname verification stays mandatory
    r.url=origin+"/post";r.method="POST";r.body="request-body";std::string body;auto post=perform(r,[&](auto p,auto n,const auto&){body.append(p,n);});assert(post.status==200&&body==r.body);
    r.method="GET";r.url=origin+"/good";bool threw=false;try{perform(r,[](auto,auto,const auto&){throw std::runtime_error("sink failure");});}catch(const std::runtime_error& e){threw=std::string(e.what())=="sink failure";}assert(threw);
    r.url=origin+"/truncated";rejects(r);
    r.url=origin+"/paced";r.limit=8192*20;r.timeout_ms=80;r.streaming=false;rejects(r);
    r.streaming=true;body.clear();auto streamed=perform(r,[&](auto p,auto n,const auto&){body.append(p,n);});
    assert(streamed.status==200&&body==std::string(8192*20,'p'));
    r.url=origin+"/stalled";const auto cancel_at=std::chrono::steady_clock::now()+std::chrono::milliseconds(150);
    r.cancelled=[&]{return std::chrono::steady_clock::now()>=cancel_at;};rejects(r);
    puts("PASS real HTTPS: trusted chain/hostname, redirects/status/POST, bounds/cancellation/truncation/sink unwinding; active streaming outlives metadata deadline and stalled streaming remains cancellable");
}
