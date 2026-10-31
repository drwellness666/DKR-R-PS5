// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_https.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <set>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
namespace fixture {
dkrr::https::Request sent;dkrr::https::Response response;std::string body="{}";
void reset(){sent={};response={200,{{"content-type","application/json"},{"content-length","2"}},0};body="{}";}
}
namespace dkrr::https {
Response perform(const Request& request,const Sink& sink) {
 fixture::sent=request;if(request.cancelled&&request.cancelled())throw std::runtime_error("cancelled");
 auto response=fixture::response;
 for(char c:fixture::body){if(++response.received>request.limit)throw std::runtime_error("too large");if(sink)sink(&c,1,response);}
 return response;
}
}
namespace dkr::store {
constexpr const char* Api="https://api.example.test";constexpr std::uint64_t MaxImage=8*1024*1024;
struct Error:std::runtime_error{using std::runtime_error::runtime_error;};
struct Request{std::string url="https://api.example.test/library",method="GET",body,token,type="application/json",etag,online_instance;std::uint64_t offset=0,limit=4096;unsigned timeout=12000;};
struct Response{unsigned status=0;std::string body,type,etag,range,location,url;std::uint64_t length=0;};
using Sink=std::function<void(const char*,std::size_t,const Response&)>;
bool prefix(std::string_view a,std::string_view b){return a.starts_with(b);}
bool trusted_url(const std::string& url,bool){return url.starts_with("https://api.example.test/")||url.starts_with("https://files.example.test/");}
bool identifier(std::string_view value){return value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==value.npos;}
std::string lower(std::string s){for(auto&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
#include "ps5_store_curl.inl"
}
template<class F>void rejects(F&& action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}assert(failed);}
int main(){using namespace dkr::store;fixture::reset();Request r;
 auto result=http_once(r,{},{});assert(result.status==200&&result.body=="{}"&&result.type=="application/json");
 assert(!fixture::sent.streaming);
 http_once(r,{},[](auto,auto,const auto&){});assert(!fixture::sent.streaming); // API remains deadline-bound.
 r.token=std::string(43,'a');r.online_instance="directory-instance";r.method="POST";r.body="{}";r.etag="etag";r.offset=17;http_once(r,{},{});
 auto header=[](const char* key){for(const auto& [name,value]:fixture::sent.headers)if(name==key)return value;return std::string{};};
 assert(header("Authorization")=="Bearer "+r.token&&header("X-DKRR-Instance")==r.online_instance&&header("Range")=="bytes=17-"&&header("If-Range")=="etag");
 r.url="https://files.example.test/package";rejects([&]{http_once(r,{},{});});r.token.clear();http_once(r,{},{});assert(header("Authorization").empty()&&header("X-DKRR-Instance").empty());
 r.method="GET";http_once(r,{},[](auto,auto,const auto&){});assert(fixture::sent.streaming);
 http_once(r,{},{});assert(!fixture::sent.streaming); // Cover/metadata buffering is not a pack stream.
 r.url="https://invalid.example.test/";rejects([&]{http_once(r,{},{});});r.url="https://api.example.test/library";
 fixture::response.headers.emplace_back("content-length","2");rejects([&]{http_once(r,{},{});});fixture::reset();
 fixture::body="x";rejects([&]{http_once(r,{},{});});fixture::reset();
 fixture::response.headers={{"content-length","9999999"}};rejects([&]{http_once(r,{},{});});fixture::reset();
 std::stop_source stop;stop.request_stop();rejects([&]{http_once(r,stop.get_token(),{});});
 rejects([&]{http_once(r,{},[](auto,auto,const auto&){throw std::runtime_error("sink failed");});});
 puts("PASS native Store transport: authenticated directory header restricted to API, file-host credential refusal, methods/ranges, duplicate/oversized/truncated headers, cancellation and sink errors");
}
