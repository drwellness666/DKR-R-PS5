// SPDX-License-Identifier: GPL-3.0-or-later
Response http_once(const Request& r,std::stop_token stop,const Sink& sink) {
    if(!trusted_url(r.url,prefix(r.url,Api)))throw Error("Unapproved library address.");
    dkrr::https::Request request;request.url=r.url;request.method=r.method;request.body=r.body;request.timeout_ms=r.timeout;request.limit=r.limit;request.cancelled=[&]{return stop.stop_requested();};
    request.streaming=bool(sink)&&r.method=="GET"&&!prefix(r.url,Api)&&r.token.empty();
    if(prefix(r.url,Api)) {
        request.headers.emplace_back("Origin","https://positron-portal.com");
        if(!r.online_instance.empty())request.headers.emplace_back("X-DKRR-Instance",r.online_instance);
        if(!r.token.empty()) {
            if(r.token.size()!=43||!identifier(r.token))throw Error("Invalid library credentials.");
            request.headers.emplace_back("Authorization","Bearer "+r.token);
        }
    } else {
        if(!r.token.empty())throw Error("Credentials cannot be sent to a file host.");
        request.headers.emplace_back("Referer","https://positron-portal.com/dkr-r/index.html");
    }
    if(r.method=="POST")request.headers.emplace_back("Content-Type",r.type);
    if(!r.etag.empty())request.headers.emplace_back(r.offset?"If-Range":"If-None-Match",r.etag);
    if(r.offset)request.headers.emplace_back("Range","bytes="+std::to_string(r.offset)+"-");
    Response response;response.url=r.url;bool parsed=false,length_present=false;
    auto headers=[&](const dkrr::https::Response& input) {
        if(parsed)return;parsed=true;response.status=input.status;std::set<std::string> seen;
        for(const auto& [key,value]:input.headers) {
            if(key!="content-type"&&key!="etag"&&key!="content-range"&&key!="location"&&key!="content-length")continue;
            if(!seen.insert(key).second)throw Error("Ambiguous library response headers.");
            if(key=="content-type")response.type=lower(value);
            else if(key=="etag")response.etag=value;
            else if(key=="content-range")response.range=value;
            else if(key=="location")response.location=value;
            else {
                length_present=true;std::uint64_t length=0;if(value.empty())throw Error("Invalid library content length.");
                for(char c:value){if(c<'0'||c>'9'||length>(UINT64_MAX-unsigned(c-'0'))/10)throw Error("Invalid library content length.");length=length*10+unsigned(c-'0');}
                if(length>r.limit)throw Error("The library response is too large.");response.length=length;
            }
        }
    };
    const auto result=dkrr::https::perform(request,[&](const char* data,std::size_t size,const dkrr::https::Response& input) {
        headers(input);
        if(sink&&response.status>=200&&response.status<300)sink(data,size,response);
        else if(size<=std::min<std::uint64_t>(r.limit,MaxImage)-std::min<std::uint64_t>(std::min<std::uint64_t>(r.limit,MaxImage),response.body.size()))response.body.append(data,size);
        else throw Error("The library response is too large.");
    });
    headers(result);if(length_present&&result.received!=response.length)throw Error("The library download is incomplete.");return response;
}
