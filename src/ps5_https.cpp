// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_https.hpp"
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#ifndef DKRR_TLS_ROOTS_HEADER
#define DKRR_TLS_ROOTS_HEADER "ps5_tls_roots.generated.h"
#endif
#include DKRR_TLS_ROOTS_HEADER
namespace dkrr::https {
const char* trusted_roots(){return dkrr_tls_roots;}
std::size_t trusted_roots_size(){return sizeof(dkrr_tls_roots)-1;}
bool available(){
    static std::once_flag once;static bool good=false;
    std::call_once(once,[]{
        // All returned buffers must belong to the same title heap. A system
        // strdup allocates inside libc and bypasses the title's malloc wrap.
        const auto duplicate=+[](const char* value)->char*{const auto n=std::strlen(value)+1;auto* p=static_cast<char*>(std::malloc(n));if(p)std::memcpy(p,value,n);return p;};
        good=curl_global_init_mem(CURL_GLOBAL_DEFAULT,std::malloc,std::free,std::realloc,duplicate,std::calloc)==CURLE_OK;
        if(good){auto* multi=curl_multi_init();std::fprintf(stderr,"[ps5][https] init=ready multi=%s\n",multi?"ready":"failed");if(multi)curl_multi_cleanup(multi);}
    });return good;
}
namespace {
struct Transfer {
    const Request& request; const Sink& sink; Response response;
    std::size_t header_bytes=0;std::exception_ptr failure;
    bool cancelled() const { return request.cancelled&&request.cancelled(); }
    template<class F> std::size_t protect(F&& fn) noexcept {
        try { if(cancelled())throw std::runtime_error("Request cancelled.");return fn(); }
        catch(...){failure=std::current_exception();return 0;}
    }
};
std::size_t header(char* raw,std::size_t size,std::size_t count,void* opaque) noexcept {
    auto& t=*static_cast<Transfer*>(opaque);const auto n=size*count;
    return t.protect([&] {
        if(n>32768-t.header_bytes)throw std::runtime_error("HTTPS response headers are too large.");
        t.header_bytes+=n;std::string_view line(raw,n);
        if(line.starts_with("HTTP/")) {
            const auto space=line.find(' ');
            if(space==line.npos||line.size()<space+4)throw std::runtime_error("Invalid HTTP status.");
            unsigned status=0;for(unsigned i=1;i<=3;++i){const char c=line[space+i];if(c<'0'||c>'9')throw std::runtime_error("Invalid HTTP status.");status=status*10+unsigned(c-'0');}
            if(status<100||status>599)throw std::runtime_error("Invalid HTTP status.");
            t.response.status=status;t.response.headers.clear();return n;
        }
        if(line=="\r\n"||line=="\n")return n;
        if(line.empty()||line.front()==' '||line.front()=='\t')throw std::runtime_error("Invalid HTTP header folding.");
        const auto colon=line.find(':');if(colon==line.npos)throw std::runtime_error("Invalid HTTP response header.");
        std::string key(line.substr(0,colon));
        for(char& c:key){if(!std::isalnum(static_cast<unsigned char>(c))&&c!='-'&&c!='_')throw std::runtime_error("Invalid HTTP response header name.");c=char(std::tolower(static_cast<unsigned char>(c)));}
        auto value=line.substr(colon+1);while(!value.empty()&&(value.front()==' '||value.front()=='\t'))value.remove_prefix(1);
        while(!value.empty()&&(value.back()=='\r'||value.back()=='\n'||value.back()==' '||value.back()=='\t'))value.remove_suffix(1);
        if(value.find_first_of("\r\n")!=value.npos)throw std::runtime_error("Invalid HTTP response header value.");
        t.response.headers.emplace_back(std::move(key),std::string(value));return n;
    });
}
std::size_t body(char* raw,std::size_t size,std::size_t count,void* opaque) noexcept {
    auto& t=*static_cast<Transfer*>(opaque);const auto n=size*count;
    return t.protect([&]{if(n>t.request.limit-std::min(t.request.limit,t.response.received))throw std::runtime_error("HTTPS response is too large.");t.response.received+=n;if(t.sink)t.sink(raw,n,t.response);return n;});
}
int progress(void* opaque,curl_off_t,curl_off_t,curl_off_t,curl_off_t) noexcept {
    auto& t=*static_cast<Transfer*>(opaque);
    try{return t.cancelled()?1:0;}catch(...){t.failure=std::current_exception();return 1;}
}
}
Response perform(const Request& r,const Sink& sink) {
    if(!available())throw std::runtime_error("HTTPS could not initialize.");
    if(!r.url.starts_with("https://")||r.url.find_first_of("\r\n")!=r.url.npos||(r.method!="GET"&&r.method!="POST"))throw std::runtime_error("Invalid HTTPS request.");
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> easy(curl_easy_init(),curl_easy_cleanup);
    if(!easy)throw std::runtime_error("HTTPS connection could not initialize.");
    auto* c=easy.get();Transfer transfer{r,sink,{},0,{}};
    auto option=[&](CURLoption name,auto value){if(curl_easy_setopt(c,name,value)!=CURLE_OK)throw std::runtime_error("HTTPS configuration failed.");};
    curl_blob roots{const_cast<char*>(trusted_roots()),trusted_roots_size(),CURL_BLOB_NOCOPY};
    option(CURLOPT_URL,r.url.c_str());option(CURLOPT_PROTOCOLS_STR,"https");option(CURLOPT_REDIR_PROTOCOLS_STR,"https");
    option(CURLOPT_FOLLOWLOCATION,0L);option(CURLOPT_PROXY,"");option(CURLOPT_NOSIGNAL,1L);
    option(CURLOPT_SSL_VERIFYPEER,1L);option(CURLOPT_SSL_VERIFYHOST,2L);option(CURLOPT_CAINFO_BLOB,&roots);
    option(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));option(CURLOPT_USERAGENT,r.user_agent.c_str());
    option(CURLOPT_CONNECTTIMEOUT_MS,r.streaming?15000L:4000L);
    option(CURLOPT_TIMEOUT_MS,r.streaming?0L:long(std::clamp(r.timeout_ms,1U,600000U)));
    if(r.streaming){
        // A healthy multi-gigabyte pack must not be cut off by the Store's
        // metadata deadline (15 seconds). Abort a stalled/near-stalled stream
        // instead; the Store retains verified resume metadata and byte caps.
        option(CURLOPT_LOW_SPEED_LIMIT,1024L);option(CURLOPT_LOW_SPEED_TIME,30L);
    }
    option(CURLOPT_NOPROGRESS,0L);option(CURLOPT_XFERINFOFUNCTION,progress);option(CURLOPT_XFERINFODATA,&transfer);
    option(CURLOPT_HEADERFUNCTION,header);option(CURLOPT_HEADERDATA,&transfer);option(CURLOPT_WRITEFUNCTION,body);option(CURLOPT_WRITEDATA,&transfer);
    curl_slist* raw=nullptr;
    struct Cleanup {curl_slist*& list;~Cleanup(){curl_slist_free_all(list);}} cleanup{raw};
    for(const auto& [key,value]:r.headers){
        if(key.empty()||key.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-")!=key.npos||value.size()>8192||value.find_first_of("\r\n")!=value.npos)throw std::runtime_error("Invalid HTTPS request header.");
        auto* next=curl_slist_append(raw,(key+": "+value).c_str());if(!next)throw std::runtime_error("HTTPS header allocation failed.");raw=next;
    }
    if(raw)option(CURLOPT_HTTPHEADER,raw);
    if(r.method=="POST"){option(CURLOPT_POST,1L);option(CURLOPT_POSTFIELDS,r.body.data());option(CURLOPT_POSTFIELDSIZE_LARGE,curl_off_t(r.body.size()));}
    const auto result=curl_easy_perform(c);
    if(transfer.failure)std::rethrow_exception(transfer.failure);
    if(result!=CURLE_OK){std::fprintf(stderr,"[ps5][https] transport=%u streaming=%u received=%llu limit=%llu detail=%s\n",unsigned(result),unsigned(r.streaming),(unsigned long long)transfer.response.received,(unsigned long long)r.limit,curl_easy_strerror(result));throw std::runtime_error(std::string("HTTPS connection failed: ")+curl_easy_strerror(result));}
    if(transfer.response.status<200)throw std::runtime_error("HTTPS response has no final status.");
    return std::move(transfer.response);
}
}
