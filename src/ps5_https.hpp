// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace dkrr::https {
using Headers=std::vector<std::pair<std::string,std::string>>;
struct Response { unsigned status=0; Headers headers; std::uint64_t received=0; };
struct Request {
    std::string url,method="GET",body,user_agent="DKR-R-PS5/1.0.9";
    Headers headers;
    unsigned timeout_ms=30000;
    std::uint64_t limit=1024*1024;
    std::function<bool()> cancelled;
    // File downloads have a progress/stall policy; metadata/API calls keep
    // their short absolute deadline. Byte limits and cancellation apply to both.
    bool streaming=false;
};
using Sink=std::function<void(const char*,std::size_t,const Response&)>;
bool available();
Response perform(const Request&,const Sink&);
const char* trusted_roots();
std::size_t trusted_roots_size();
}
