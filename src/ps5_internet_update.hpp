// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
namespace dkrr::updates {
constexpr std::size_t manifest_limit=1024;
constexpr std::uint64_t payload_limit=128ULL*1024*1024;
struct Offer {
    unsigned sequence{};
    std::uint64_t bytes{};
    char tag[64]{};
    unsigned char sha256[32]{};
    bool native_package{};
};
// Exact, signed wire format. No URLs or filesystem paths come from the feed.
bool verify_offer(const char*,std::size_t,const unsigned char key[32],Offer&);
bool allowed_asset_url(const char*,bool redirected);
bool payload_digest(int,const Offer&);
enum class State : unsigned { Idle,Checking,Current,Available,Downloading,HandingOff,Closing,Unavailable,Failed };
struct View { State state{}; unsigned sequence{}; std::uint64_t received{},bytes{}; char message[192]{};
    std::uint64_t space_required{},space_available{}; bool space_known{}; };
void initialize();
void check();
void install();
void cancel();
void shutdown();
View view();
bool close_requested();
bool automatic_check();
bool set_automatic_check(bool);
}
