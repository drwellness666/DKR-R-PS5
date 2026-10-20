// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>
#include "ps5_store_random.hpp"
static int random_status=0;
extern "C" int sceRandomGetRandomNumber(std::uint8_t* out,std::size_t size){assert(size==32);for(std::size_t i=0;i<size;++i)out[i]=std::uint8_t(i+1);return random_status;}
namespace fixture {
int fail_at=0, calls=0, live=0, method=-1;
std::string headers, body;
std::map<std::string,std::string> sent;
bool fail(){return ++calls==fail_at;}
int create(){if(fail())return -1;++live;return live;}
void reset(std::string h="Content-Type: application/json\r\nContent-Length: 2\r\n",std::string b="{}") {
    assert(live==0);fail_at=0;calls=0;method=-1;sent.clear();headers=std::move(h);body=std::move(b);
}
}
extern "C" {
int sceNetPoolCreate(const char*,int,int){return fixture::create();}
int sceSslInit(std::size_t){return fixture::create();}
int sceHttpInit(int,int,std::size_t){return fixture::create();}
int sceHttpCreateTemplate(int,const char*,int,int){return fixture::create();}
int sceHttpCreateConnectionWithURL(int,const char*,int){return fixture::create();}
int sceHttpCreateRequestWithURL(int,int m,const char*,std::uint64_t){fixture::method=m;return fixture::create();}
int sceNetPoolDestroy(int){--fixture::live;return 0;}
int sceSslTerm(int){--fixture::live;return 0;}
int sceHttpTerm(int){--fixture::live;return 0;}
int sceHttpDeleteTemplate(int){--fixture::live;return 0;}
int sceHttpDeleteConnection(int){--fixture::live;return 0;}
int sceHttpDeleteRequest(int){--fixture::live;return 0;}
int sceHttpSetAutoRedirect(int,int v){assert(v==0);return fixture::fail()?-1:0;}
int sceHttpsEnableOption(int,std::uint32_t mask){assert(mask==0xBD);return fixture::fail()?-1:0;}
int sceHttpSetResolveTimeOut(int,std::uint32_t){return fixture::fail()?-1:0;}
int sceHttpSetConnectTimeOut(int,std::uint32_t){return fixture::fail()?-1:0;}
int sceHttpSetSendTimeOut(int,std::uint32_t){return fixture::fail()?-1:0;}
int sceHttpSetRecvTimeOut(int,std::uint32_t){return fixture::fail()?-1:0;}
int sceHttpAddRequestHeader(int,const char* n,const char* v,std::uint32_t){fixture::sent[n]=v;return fixture::fail()?-1:0;}
int sceHttpSendRequest(int,const void*,std::size_t){return fixture::fail()?-1:0;}
int sceHttpGetStatusCode(int,int* s){*s=200;return fixture::fail()?-1:0;}
int sceHttpGetAllResponseHeaders(int,char** p,std::size_t* n){*p=fixture::headers.data();*n=fixture::headers.size();return fixture::fail()?-1:0;}
int sceHttpReadData(int,void* p,std::size_t n){if(fixture::fail())return -1;const auto count=std::min(n,fixture::body.size());std::copy_n(fixture::body.data(),count,static_cast<char*>(p));fixture::body.erase(0,count);return int(count);}
}
namespace dkr::store {
using Clock=std::chrono::steady_clock;
constexpr const char* Api="https://example.test";
constexpr std::uint64_t MaxImage=8*1024*1024;
struct Error:std::runtime_error{using std::runtime_error::runtime_error;};
struct Request{std::string url="https://example.test/data",method="GET",body,token,type="application/json",etag;std::uint64_t offset=0,limit=4096;unsigned timeout=12000;};
struct Response{unsigned status=0;std::string body,type,etag,range,location,url;std::uint64_t length=0;};
using Sink=std::function<void(const char*,std::size_t,const Response&)>;
bool prefix(std::string_view a,std::string_view b){return a.starts_with(b);}
bool trusted_url(const std::string& url,bool){return url.starts_with("https://example.test/");}
std::string lower(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
#include "ps5_store_http.inl"
}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught&&fixture::live==0);}
int main(){
    using namespace dkr::store;
    std::array<std::uint8_t,32> nonce{};assert(dkrr::ps5_store::nonce_bytes(nonce)&&nonce[0]==1&&nonce[31]==32);
    for(int status:{-1,1}){random_status=status;assert(!dkrr::ps5_store::nonce_bytes(nonce));for(auto byte:nonce)assert(byte==0);}random_status=0;
    fixture::reset();auto response=http_once(Request{},{},{});assert(response.status==200&&response.body=="{}"&&fixture::method==0&&fixture::live==0);const auto success_calls=fixture::calls;
    for(int point=1;point<=success_calls;++point){fixture::reset();fixture::fail_at=point;rejects([]{http_once(Request{},{},{});});}
    fixture::reset();Request post;post.method="POST";post.body="{}";post.token="test-token";http_once(post,{},{});assert(fixture::method==2&&fixture::sent.at("Authorization")=="Bearer test-token"&&fixture::live==0);
    fixture::reset();Request range;range.offset=12;range.etag="test-etag";http_once(range,{},{});assert(fixture::sent.at("Range")=="bytes=12-"&&fixture::sent.at("If-Range")=="test-etag");
    fixture::reset();Request bad;bad.token="bad\r\nInjected: yes";rejects([&]{http_once(bad,{},{});});
    fixture::reset();bad=Request{};bad.url="http://example.test/";rejects([&]{http_once(bad,{},{});});assert(fixture::calls==0);
    fixture::reset();std::stop_source stop;stop.request_stop();rejects([&]{http_once(Request{},stop.get_token(),{});});assert(fixture::calls==0);
    for(const auto& header:std::vector<std::string>{"Content-Length: 3\r\n","Content-Length: 9000\r\n","Content-Length: -1\r\n","Content-Length: 18446744073709551616\r\n","Content-Length: 2\r\nContent-Length: 2\r\n",std::string(32769,'a')}){fixture::reset(header);rejects([]{http_once(Request{},{},{});});}
    fixture::reset();rejects([]{http_once(Request{},{},[](const char*,std::size_t,const Response&){throw Error("disk full");});});
    fixture::reset("Content-Type: application/json\r\n",std::string(5000,'a'));rejects([]{http_once(Request{},{},{});});
}
