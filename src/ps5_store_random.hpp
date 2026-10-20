// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
// Signature and zero-success result follow the public payload SDK's
// samples/hello_sprx and samples/hello_so. The platform's timestamp-seeded
// ps5_arc4random fallback is deliberately unsuitable for an OAuth verifier.
extern "C" int sceRandomGetRandomNumber(std::uint8_t*,std::size_t) __attribute__((weak));
namespace dkrr::ps5_store {
inline bool nonce_bytes(std::array<std::uint8_t,32>& bytes) {
    if(sceRandomGetRandomNumber && sceRandomGetRandomNumber(bytes.data(),bytes.size())==0)return true;
    bytes.fill(0);
    return false;
}
}
