// SPDX-License-Identifier: GPL-3.0-or-later
// Enclosing update worker owns URL, signed offer and transaction policies.
struct Services { bool start() { return dkrr::https::available(); } };
bool download(Services&,const Offer& release,int fd) {
    std::string url="https://github.com/drwellness666/DKR-R-PS5/releases/download/"+std::string(release.tag)+"/DKRR-Update.dkru";
    const auto deadline=Clock::now()+std::chrono::minutes(10);
    for(unsigned hop=0;hop<4&&!stopping.load();++hop) {
        if(!allowed_asset_url(url.c_str(),hop!=0))return false;
        auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-Clock::now()).count();if(remaining<=0)return false;
        dkrr::https::Request request;request.url=url;request.limit=release.bytes;request.timeout_ms=unsigned(remaining);request.cancelled=[]{return stopping.load();};
        auto response=dkrr::https::perform(request,[&](const char* data,std::size_t count,const dkrr::https::Response& response) {
            if(response.status!=200)return;
            if(!write_all(fd,reinterpret_cast<const unsigned char*>(data),count))throw std::runtime_error("Update download could not be saved.");
            std::lock_guard lock(mutex);current.received=response.received;
        });
        if(response.status==301||response.status==302||response.status==303||response.status==307||response.status==308) {
            unsigned matches=0;std::string next;
            for(const auto& [key,value]:response.headers)if(key=="location"){++matches;next=value;}
            if(matches!=1||next.size()>=2048||!allowed_asset_url(next.c_str(),true))return false;
            url=std::move(next);continue;
        }
        return response.status==200&&response.received==release.bytes&&payload_digest(fd,release);
    }
    return false;
}
