// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_menu_pointer.hpp"
#include <imgui_internal.h>
#include <cassert>
#include <cstdio>
static unsigned clicks=0;
static float wheel_x=0,wheel_y=0;
static void frame(float x=0,float y=0,bool cross=false,bool navigation=false,bool connected=true,
                  float scroll_x=0,float scroll_y=0) {
    auto& io=ImGui::GetIO();
    dkrr::menu_pointer::state.update(io,x,y,cross,navigation,connected,scroll_x,scroll_y);
    ImGui::NewFrame();
    wheel_x=io.MouseWheelH;wheel_y=io.MouseWheel;
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Pointer",nullptr,ImGuiWindowFlags_NoDecoration);
    ImGui::SetCursorPos({400,300});
    if(ImGui::Button("Click",{200,120}))++clicks;
    ImGui::End();dkrr::menu_pointer::draw();ImGui::Render();
}
int main() {
    using namespace dkrr::menu_pointer;
    ImGui::CreateContext();auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;io.DisplaySize={1000,720};io.DeltaTime=1.F/60;
    unsigned char*p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
    frame(1);assert(!state.active&&!io.MouseDrawCursor);
    enabled=true;visible=true;frame(.1F);assert(!state.active);
    frame(.3F);assert(state.active&&state.position.x>500&&state.position.x<501);
    assert(ImGui::GetDrawData()->CmdListsCount>=2); // themed foreground cursor submitted
    frame();frame(0,0,true);frame(0,0,true);frame();frame();assert(clicks==1);
    frame(0,0,true);frame(0,0,false,true);frame();assert(!io.MouseDown[0]&&!state.active&&!io.MouseDrawCursor);
    for(int i=0;i<400;++i)frame(1,1);assert(state.position.x==999&&state.position.y==719);
    for(int i=0;i<400;++i)frame(-1,-1);assert(state.position.x==0&&state.position.y==0);
    state.position={500,360};state.velocity={};io.DisplaySize={2000,1440};frame();assert(state.position.x==1000&&state.position.y==720);
    frame(1);frame(0,0,true);visible=false;frame();frame();assert(!io.MouseDown[0]&&!state.active);
    visible=true;frame(1);frame(0,0,true);frame(0,0,false,false,false);frame();assert(!io.MouseDown[0]&&!state.active);
    enabled=false;frame(1);assert(!state.active);
    enabled=true;visible=true;
    auto distance=[&](int rate) {
        set_speed(rate);state.position={400,300};state.velocity={};state.active=false;
        for(int i=0;i<20;++i)frame(1);
        return state.position.x-400;
    };
    const float slow=distance(50),fast=distance(100);
    assert(slow>0&&std::abs(fast/slow-2.0F)<.001F);
    set_speed(-1);assert(speed()==50);set_speed(10000);assert(speed()==200);set_speed(115);
    state.position={500,360};state.velocity={};frame(1);frame();
    frame(0,0,false,false,true,.1F,.1F);assert(!wheel_x&&!wheel_y);
    frame(0,0,false,false,true,1,1);assert(wheel_x>0&&wheel_y<0);
    frame(0,0,false,false,true,-1,-1);assert(wheel_x<0&&wheel_y>0);
    const auto wheel_distance=[&](int frequency) {
        io.DeltaTime=1.F/float(frequency);float sum=0;
        for(int i=0;i<frequency;++i){frame(0,0,false,false,true,0,1);sum+=wheel_y;}
        return sum;
    };
    const float at60=wheel_distance(60),at120=wheel_distance(120);
    assert(std::abs(at60+8.F)<.0001F&&std::abs(at120-at60)<.0001F);
    frame(0,0,false,true,true,0,1);assert(!wheel_y&&!state.active);
    frame(0,0,false,false,true,0,1);assert(!wheel_y); // No wheel in gamepad navigation.
    frame(1);visible=false;frame(0,0,false,false,true,0,1);assert(!wheel_y);
    visible=true;frame(1);frame(0,0,false,false,false,0,1);assert(!wheel_y);
    io.DeltaTime=1.F/60;
    enabled=false;
    ImGui::DestroyContext();state={};ImGui::CreateContext();auto& next=ImGui::GetIO();
    next.IniFilename=nullptr;next.DisplaySize={800,600};next.DeltaTime=1.F/60;next.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
    frame();assert(!state.click&&state.position.x==400&&state.position.y==300);
    ImGui::DestroyContext();puts("PASS actual ImGui pointer: dead zone, Cross widget click, right-stick wheel direction/dead zone/60-120 Hz rate, navigation handoff, disconnect/overlay release, viewport scale and fresh context");
}
