// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only native C++20 runtime diagnostic; output goes to the ELF loader.
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <semaphore>
#include <stdexcept>
#include <thread>

static thread_local int local_value = 7;
int main() {
    std::atomic<int> result{0};
    std::binary_semaphore ready{0};
    local_value = 11;
    std::thread worker([&] {
        local_value = 23;
        result.store(local_value, std::memory_order_release);
        ready.release();
    });
    ready.acquire();
    worker.join();
    bool threads_ok=result.load(std::memory_order_acquire)==23 && local_value==11;
    bool exceptions_ok=false;
    try {throw std::runtime_error("PS5 C++ diagnostic");}
    catch(const std::runtime_error&) {exceptions_ok=true;}
    std::error_code ec;
    auto data=std::filesystem::status("/data",ec);
    std::printf("DKR-R PS5 C++ PROBE: threads/semaphore/TLS=%s exceptions=%s /data=%s (ec=%d)\n",
        threads_ok?"PASS":"FAIL",exceptions_ok?"PASS":"FAIL",
        std::filesystem::is_directory(data)?"directory":"unavailable",ec.value());
    std::fflush(stdout);
    return threads_ok && exceptions_ok ? 0 : 1;
}
