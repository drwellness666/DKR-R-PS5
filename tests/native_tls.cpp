// SPDX-License-Identifier: GPL-3.0-or-later
#include <thread>
#include <string>
#include <atomic>
#include <cassert>
#include <cstdio>
static std::atomic<unsigned> destroyed;
struct Value {
    std::string text=std::string(200,'x');
    ~Value() { assert(text.size()==200);++destroyed; }
};
thread_local Value value;
int main() {
    for(unsigned i=0;i<32;++i) {
        std::thread thread([] { assert(value.text.size()==200); });thread.join();
    }
    assert(destroyed==32);
    std::puts("PASS TLS: 32 C++ destructors with emulated TLS and AddressSanitizer");
}
