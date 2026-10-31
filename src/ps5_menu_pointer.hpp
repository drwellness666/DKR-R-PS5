// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cfloat>
namespace dkrr::menu_pointer {
inline std::atomic_bool enabled{false},visible{false};
inline std::atomic_int speed_percent{115};
inline int speed() { return std::clamp(speed_percent.load(),50,200); }
inline void set_speed(int value) { speed_percent.store(std::clamp(value,50,200)); }
struct State {
    ImGuiContext* context=nullptr;
    ImVec2 position{},viewport{},velocity{};
    bool active=false,click=false;
    bool owns_trickle=false,saved_trickle=true;
    bool update(ImGuiIO& io,float x,float y,bool cross,bool navigation,bool connected,
                float scroll_x=0,float scroll_y=0) {
        if(context!=ImGui::GetCurrentContext()){
            context=ImGui::GetCurrentContext();position={io.DisplaySize.x*.5F,io.DisplaySize.y*.5F};viewport=io.DisplaySize;active=click=false;owns_trickle=false;
        }
        if(viewport.x>0&&viewport.y>0&&(viewport.x!=io.DisplaySize.x||viewport.y!=io.DisplaySize.y)) {
            position.x*=io.DisplaySize.x/viewport.x;position.y*=io.DisplaySize.y/viewport.y;
        }
        viewport=io.DisplaySize;
        const bool allowed=enabled.load()&&visible.load()&&connected&&io.DisplaySize.x>0&&io.DisplaySize.y>0;
        const bool previously_active=active;
        if(!allowed||navigation){active=false;velocity={};}
        const float magnitude=std::sqrt(x*x+y*y);
        ImVec2 target{};
        if(allowed&&!navigation&&magnitude>.22F) {
            active=true;
            const float strength=std::clamp((magnitude-.22F)/.78F,0.0F,1.0F);
            const float speed=io.DisplaySize.x*.30F*(float(dkrr::menu_pointer::speed())/100.0F)*strength*strength;
            target={x/magnitude*speed,y/magnitude*speed};
        }
        if(active) {
            const float dt=std::clamp(io.DeltaTime,0.0F,.05F),blend=1.0F-std::exp(-22.0F*dt);
            velocity.x+=(target.x-velocity.x)*blend;velocity.y+=(target.y-velocity.y)*blend;
            position.x+=velocity.x*dt;position.y+=velocity.y*dt;
        }
        position.x=std::clamp(position.x,0.0F,std::max(io.DisplaySize.x-1,0.0F));
        position.y=std::clamp(position.y,0.0F,std::max(io.DisplaySize.y-1,0.0F));
        // This backend submits one controller snapshot per UI frame. ImGui's
        // default event trickling otherwise separates simultaneous pointer
        // motion and wheel events into different frames and builds a backlog.
        if(active&&!owns_trickle){saved_trickle=io.ConfigInputTrickleEventQueue;owns_trickle=true;io.ConfigInputTrickleEventQueue=false;}
        else if(!active&&owns_trickle){io.ConfigInputTrickleEventQueue=saved_trickle;owns_trickle=false;}
        io.MouseDrawCursor=false; // Draw the themed pointer at EndFramePre instead.
        if(active)io.AddMousePosEvent(position.x,position.y);
        else if(previously_active)io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
        // Mouse navigation has no gamepad scroll focus. Feed the same wheel
        // path as a real mouse at the pointer, with a time-based rate so
        // 60/120 Hz presentation does not change scrolling speed.
        if(active&&!navigation) {
            const auto wheel=[](float value) {
                const float strength=std::clamp((std::abs(value)-.22F)/.78F,0.0F,1.0F);
                return std::copysign(strength*strength,value);
            };
            const float rate=8.0F*std::clamp(io.DeltaTime,0.0F,.05F);
            const float horizontal=wheel(scroll_x)*rate,vertical=-wheel(scroll_y)*rate;
            if(horizontal||vertical)io.AddMouseWheelEvent(horizontal,vertical);
        }
        const bool pressed=active&&cross;
        if(pressed!=click){io.AddMouseButtonEvent(0,pressed);click=pressed;}
        return active;
    }
};
inline State state;
inline void draw() {
    if(!state.active||!visible.load()||!enabled.load())return;
    const auto p=state.position;auto* list=ImGui::GetForegroundDrawList();
    // Golden pointer with the menu's red outline and a small racing checker.
    // Its tip is the exact hit position; no copyrighted image asset is needed.
    const ImVec2 outline[]={{p.x,p.y},{p.x+3,p.y+31},{p.x+10,p.y+23},{p.x+16,p.y+34},{p.x+22,p.y+31},{p.x+16,p.y+20},{p.x+27,p.y+20}};
    list->AddTriangleFilled(outline[0],outline[1],outline[2],IM_COL32(92,20,16,255));
    list->AddTriangleFilled(outline[0],outline[2],outline[6],IM_COL32(92,20,16,255));
    list->AddQuadFilled(outline[2],outline[3],outline[4],outline[5],IM_COL32(92,20,16,255));
    list->AddTriangleFilled(outline[2],outline[5],outline[6],IM_COL32(92,20,16,255));
    const ImVec2 face[]={{p.x+2,p.y+3},{p.x+5,p.y+25},{p.x+11,p.y+19},{p.x+18,p.y+30},{p.x+19,p.y+29},{p.x+13,p.y+17},{p.x+22,p.y+18}};
    // The arrow is concave, so use explicit triangles rather than a convex fan.
    list->AddTriangleFilled(face[0],face[1],face[2],IM_COL32(255,220,69,255));
    list->AddTriangleFilled(face[0],face[2],face[6],IM_COL32(255,220,69,255));
    list->AddQuadFilled(face[2],face[3],face[4],face[5],IM_COL32(255,220,69,255));
    list->AddLine({p.x+3,p.y+5},{p.x+5,p.y+20},IM_COL32(255,255,226,255),1.5F);
}

}
