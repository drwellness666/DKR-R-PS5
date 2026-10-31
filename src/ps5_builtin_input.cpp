// SPDX-License-Identifier: GPL-3.0-or-later
// The PS5 uses the in-process SDL/ScePad backend, including all DualSense
// effects. The optional desktop SDL3 helper executable is not a console API.
#include "sdl3_input_client.hpp"
namespace dkr::runtime::sdl3_input {
struct Client::Impl {};
Client::Client():impl_(std::make_unique<Impl>()){}
Client::~Client()=default;
bool Client::start(const std::filesystem::path&,const std::filesystem::path&,
                   std::string& error,std::stop_token){
    error="PS5 controllers use the built-in native input backend.";return false;
}
void Client::stop(){}
bool Client::healthy()const{return false;}
std::string Client::detail()const{return "PS5 controllers use the built-in native input backend.";}
State Client::state()const{return {};}
bool Client::rumble(int,std::uint16_t,std::uint16_t,std::uint32_t){return false;}
}
