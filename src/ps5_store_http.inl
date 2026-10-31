#if defined(DKRR_STATIC_HTTPS)
#include "ps5_store_curl.inl"
#else
// SPDX-License-Identifier: GPL-3.0-or-later
// Included inside dkr::store's implementation namespace. The enclosing
// upstream code owns URL allowlists, redirect limits and credential policy.
// Native service sequence follows the GPL ps5-native-app-boilerplate example
// by BlackBearReloaded, as in ps5_internet_update.cpp.
extern "C" {
int sceNetPoolCreate(const char*,int,int) __attribute__((weak));
int sceNetPoolDestroy(int) __attribute__((weak));
int sceSslInit(std::size_t) __attribute__((weak));
int sceSslTerm(int) __attribute__((weak));
int sceHttpInit(int,int,std::size_t) __attribute__((weak));
int sceHttpTerm(int) __attribute__((weak));
int sceHttpCreateTemplate(int,const char*,int,int) __attribute__((weak));
int sceHttpDeleteTemplate(int) __attribute__((weak));
int sceHttpSetAutoRedirect(int,int) __attribute__((weak));
int sceHttpSetResolveTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpSetConnectTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpSetSendTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpSetRecvTimeOut(int,std::uint32_t) __attribute__((weak));
int sceHttpsEnableOption(int,std::uint32_t) __attribute__((weak));
int sceHttpCreateConnectionWithURL(int,const char*,int) __attribute__((weak));
int sceHttpDeleteConnection(int) __attribute__((weak));
int sceHttpCreateRequestWithURL(int,int,const char*,std::uint64_t) __attribute__((weak));
int sceHttpDeleteRequest(int) __attribute__((weak));
int sceHttpAddRequestHeader(int,const char*,const char*,std::uint32_t) __attribute__((weak));
int sceHttpSendRequest(int,const void*,std::size_t) __attribute__((weak));
int sceHttpGetStatusCode(int,int*) __attribute__((weak));
int sceHttpGetAllResponseHeaders(int,char**,std::size_t*) __attribute__((weak));
int sceHttpReadData(int,void*,std::size_t) __attribute__((weak));
}
Response http_once(const Request& r, std::stop_token stop, const Sink& sink) {
    if (!trusted_url(r.url, prefix(r.url, Api))) throw Error("Unapproved library address.");
    if (r.method != "GET" && r.method != "POST") throw Error("Unsupported library request method.");
    if (!sceNetPoolCreate || !sceNetPoolDestroy || !sceSslInit || !sceSslTerm ||
        !sceHttpInit || !sceHttpTerm || !sceHttpCreateTemplate || !sceHttpDeleteTemplate ||
        !sceHttpSetAutoRedirect || !sceHttpSetResolveTimeOut || !sceHttpSetConnectTimeOut ||
        !sceHttpSetSendTimeOut || !sceHttpSetRecvTimeOut || !sceHttpsEnableOption ||
        !sceHttpCreateConnectionWithURL || !sceHttpDeleteConnection ||
        !sceHttpCreateRequestWithURL || !sceHttpDeleteRequest || !sceHttpAddRequestHeader ||
        !sceHttpSendRequest || !sceHttpGetStatusCode || !sceHttpGetAllResponseHeaders || !sceHttpReadData)
        throw Error("Native HTTPS is unavailable. The installed game is unchanged.");
    struct Session {
        int pool=-1, ssl=-1, http=-1, tmpl=-1, connection=-1, request=-1;
        ~Session() {
            if(request>=0)sceHttpDeleteRequest(request);
            if(connection>=0)sceHttpDeleteConnection(connection);
            if(tmpl>=0)sceHttpDeleteTemplate(tmpl);
            if(http>=0)sceHttpTerm(http);
            if(ssl>=0)sceSslTerm(ssl);
            if(pool>=0)sceNetPoolDestroy(pool);
        }
    } s;
    const auto deadline=Clock::now()+std::chrono::milliseconds(r.timeout);
    auto cancelled=[&] {
        if(stop.stop_requested() || Clock::now()>deadline) throw Error("Library request cancelled or timed out.");
    };
    cancelled();
    if((s.pool=sceNetPoolCreate("dkrr_mod_store",1024*1024,0))<0 ||
       (s.ssl=sceSslInit(304*1024))<0 || (s.http=sceHttpInit(s.pool,s.ssl,4*1024*1024))<0 ||
       (s.tmpl=sceHttpCreateTemplate(s.http,"DKR-R-PS5/1.0.8",2,0))<0 ||
       sceHttpSetAutoRedirect(s.tmpl,0)<0 || sceHttpsEnableOption(s.tmpl,0xBD)<0 ||
       sceHttpSetResolveTimeOut(s.tmpl,4000000)<0 || sceHttpSetConnectTimeOut(s.tmpl,4000000)<0 ||
       sceHttpSetSendTimeOut(s.tmpl,std::min(r.timeout,15000U)*1000)<0 ||
       sceHttpSetRecvTimeOut(s.tmpl,std::min(r.timeout,15000U)*1000)<0)
        throw Error("The secure library connection could not start.");
    if((s.connection=sceHttpCreateConnectionWithURL(s.tmpl,r.url.c_str(),0))<0 ||
       (s.request=sceHttpCreateRequestWithURL(s.connection,r.method=="POST"?2:0,r.url.c_str(),r.body.size()))<0)
        throw Error("The library request could not start.");
    auto header=[&](const char* name,const std::string& value) {
        if(value.find_first_of("\r\n")!=std::string::npos || value.size()>8192 ||
           sceHttpAddRequestHeader(s.request,name,value.c_str(),0)<0) throw Error("Invalid library request header.");
    };
    header("Accept","application/json, application/octet-stream");
    if(!r.token.empty())header("Authorization","Bearer "+r.token);
    if(!r.body.empty())header("Content-Type",r.type);
    if(!r.etag.empty())header(r.offset?"If-Range":"If-None-Match",r.etag);
    if(r.offset)header("Range","bytes="+std::to_string(r.offset)+"-");
    cancelled();
    Response response;int status=0;
    if(sceHttpSendRequest(s.request,r.body.empty()?nullptr:r.body.data(),r.body.size())<0 ||
       sceHttpGetStatusCode(s.request,&status)<0 || status<100 || status>599)
        throw Error("The library connection was interrupted. Retry to continue.");
    response.status=unsigned(status);
    char* raw=nullptr;std::size_t size=0;
    if(sceHttpGetAllResponseHeaders(s.request,&raw,&size)<0 || !raw || size>32768)
        throw Error("Invalid library response headers.");
    std::set<std::string> seen;
    for(std::string_view text(raw,size);!text.empty();) {
        const auto end=text.find('\n');auto line=text.substr(0,end);
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
        const auto colon=line.find(':');if(colon==line.npos)continue;
        const auto key=lower(std::string(line.substr(0,colon)));auto value=line.substr(colon+1);
        while(!value.empty()&&(value.front()==' '||value.front()=='\t'))value.remove_prefix(1);
        if(key!="content-type"&&key!="etag"&&key!="content-range"&&key!="location"&&key!="content-length")continue;
        if(!seen.insert(key).second)throw Error("Ambiguous library response headers.");
        if(key=="content-type")response.type=lower(std::string(value));
        else if(key=="etag")response.etag=value;
        else if(key=="content-range")response.range=value;
        else if(key=="location")response.location=value;
        else {
            std::uint64_t length=0;if(value.empty())throw Error("Invalid library content length.");
            for(char c:value) {if(c<'0'||c>'9'||length>(UINT64_MAX-unsigned(c-'0'))/10)throw Error("Invalid library content length.");length=length*10+unsigned(c-'0');}
            response.length=length;
        }
    }
    if(response.length>r.limit)throw Error("The library response is too large.");
    std::array<char,32768> buffer{};std::uint64_t total=0;
    for(;;) {
        cancelled();const int count=sceHttpReadData(s.request,buffer.data(),buffer.size());
        if(count<0 || count>int(buffer.size()))throw Error("The library download was interrupted.");
        if(!count)break;
        if(std::uint64_t(count)>r.limit-std::min(r.limit,total))throw Error("The library response is too large.");
        total+=unsigned(count);
        if(sink && response.status>=200 && response.status<300)sink(buffer.data(),std::size_t(count),response);
        else if(response.body.size()+std::size_t(count)<=std::min<std::uint64_t>(r.limit,MaxImage))response.body.append(buffer.data(),std::size_t(count));
        else throw Error("The library response is too large.");
    }
    if(seen.contains("content-length")&&total!=response.length)throw Error("The library download is incomplete.");
    return response;
}

#endif
