// SPDX-License-Identifier: GPL-3.0-or-later
// Original vector drawing: no external image, controller texture or Sony logo.
// Read-only diagram; editing uses the existing capture/undo implementation.
void DrawPs5ControllerDiagram(const dkrr::ps5_pad::Bindings& bindings,int action,float width) {
    const ImVec2 at=ImGui::GetCursorScreenPos();auto* d=ImGui::GetWindowDrawList();
    const float height=248.0F;
    const float scale=std::min(width/700.0F,height/248.0F);
    const ImVec2 center{at.x+width*0.5F,at.y+height*0.45F};
    const auto point=[&](float x,float y){return ImVec2{center.x+x*scale,center.y+y*scale};};
    const auto line=[&](float ax,float ay,float bx,float by,ImU32 c,float thick=1.0F){d->AddLine(point(ax,ay),point(bx,by),c,thick);};
    const ImU32 shell=IM_COL32(220,231,239,255),dark=IM_COL32(19,36,51,255),ink=IM_COL32(221,235,244,255),accent=IM_COL32(255,208,120,255);
    // Front view: long curved white grips, a dark centre and the wide
    // touchpad above two symmetric sticks. The shoulders are separate from
    // the face buttons; the previous capsule sketch hid that distinction.
    d->AddRectFilled(point(-118,-61),point(118,40),dark,28*scale);
    for(float side:{-1.0F,1.0F}) {
        const auto path=[&](float x,float y){return point(side*x,y);};
        d->PathLineTo(path(7,-61));
        d->PathBezierCubicCurveTo(path(70,-68),path(117,-70),path(131,-45),16);
        d->PathBezierCubicCurveTo(path(145,-14),path(165,38),path(169,71),16);
        d->PathBezierCubicCurveTo(path(169,91),path(146,100),path(132,78),12);
        d->PathBezierCubicCurveTo(path(112,44),path(87,15),path(56,15),12);
        d->PathBezierCubicCurveTo(path(29,12),path(10,-2),path(7,-61),12);
        d->PathFillConvex(shell);
        const float left=side<0?-129.0F:83.0F;
        d->AddRectFilled(point(left,-79),point(left+46,-62),dark,6*scale);
        const char* shoulder=side<0?"L1":"R1";
        d->AddText(ImGui::GetFont(),10*scale,point(left+16,-76),ink,shoulder);
        d->AddRectFilled(point(left+3,-91),point(left+43,-82),dark,4*scale);
        const char* trigger=side<0?"L2":"R2";
        d->AddText(ImGui::GetFont(),8*scale,point(left+16,-90),ink,trigger);
    }
    d->AddRectFilled(point(-64,-58),point(64,-9),dark,8*scale);
    d->AddLine(point(-62,-61),point(62,-61),IM_COL32(48,157,228,255),2*scale);
    d->AddRectFilled(point(-79,-48),point(-74,-36),dark,2*scale);
    d->AddRectFilled(point(74,-48),point(79,-36),dark,2*scale);
    d->AddCircleFilled(point(-52,17),23*scale,dark);d->AddCircleFilled(point(52,17),23*scale,dark);
    d->AddCircle(point(-52,17),16*scale,ink,24,scale);d->AddCircle(point(52,17),16*scale,ink,24,scale);
    d->AddRectFilled(point(-16,25),point(16,38),dark,5*scale);
    d->AddRectFilled(point(-112,-42),point(-100,-7),dark,2*scale);d->AddRectFilled(point(-124,-30),point(-89,-18),dark,2*scale);
    for(const auto xy:std::array<ImVec2,4>{{{110,-4},{130,-24},{90,-24},{110,-44}}})d->AddCircleFilled(point(xy.x,xy.y),9*scale,dark);
    line(106,-8,114,0,ink,scale);line(114,-8,106,0,ink,scale);
    d->AddCircle(point(130,-24),5*scale,ink,16,scale);d->AddRect(point(86,-28),point(94,-20),ink,0,0,scale);
    d->AddTriangle(point(110,-49),point(105,-40),point(115,-40),ink,scale);
    const auto label=[&](int source,float x,float y,float anchor_x,float anchor_y,bool right){
        std::string text;
        for(unsigned i=0;i<dkrr::ps5_pad::sources.size();++i)if(dkrr::ps5_pad::sources[i]==source)text=std::string(dkrr::ps5_pad::names[i])+": ";
        text+=dkrr::ps5_pad::assigned(bindings,source);const bool active=dkrr::ps5_pad::selected(bindings,source,action);
        const float font=std::clamp(15.0F*scale,11.0F,15.0F);const float room=std::max(12.0F,std::min(width*0.25F,right?at.x+width-6-point(x,y).x:point(x,y).x-at.x-6));
        while(text.size()>3&&ImGui::GetFont()->CalcTextSizeA(font,1e6F,0,text.c_str()).x>room)text.pop_back();
        const auto p=point(x,y);const float size=ImGui::GetFont()->CalcTextSizeA(font,1e6F,0,text.c_str()).x;
        const ImVec2 text_at{right?p.x:p.x-size,p.y-font*0.5F};
        d->AddText(ImGui::GetFont(),font,text_at,active?accent:ink,text.c_str());
        line(right?x-5:x+5,y,anchor_x,anchor_y,active?accent:IM_COL32(92,129,153,255),active?2:1);
    };
    label(1009,-190,-91,-100,-64,false);label(9,-190,-65,-127,-53,false);
    label(1011,190,-91,100,-64,true);label(10,190,-65,127,-53,true);
    label(11,-190,-40,-106,-41,false);label(12,-190,-13,-106,-8,false);
    label(13,-190,14,-124,-24,false);label(14,-190,41,-90,-24,false);
    label(3,190,-40,110,-44,true);label(1,190,-13,130,-24,true);
    label(0,190,14,110,-4,true);label(2,190,41,90,-24,true);
    const std::string steering=dkrr::ps5_pad::stick_label(bindings,false);d->AddText(ImGui::GetFont(),13*scale,point(-130,100),ink,steering.c_str());
    const std::string menu=dkrr::ps5_pad::stick_label(bindings,true);d->AddText(ImGui::GetFont(),13*scale,point(15,100),ink,menu.c_str());
    std::string shortcuts="Options: "+dkrr::ps5_pad::assigned(bindings,6)+"   Touchpad: "+dkrr::ps5_pad::assigned(bindings,4);
    constexpr float footer_size=11.0F;
    while(shortcuts.size()>3&&ImGui::GetFont()->CalcTextSizeA(footer_size,1e6F,0,shortcuts.c_str()).x>width-12)shortcuts.pop_back();
    const float footer_width=ImGui::GetFont()->CalcTextSizeA(footer_size,1e6F,0,shortcuts.c_str()).x;
    d->AddText(ImGui::GetFont(),footer_size,{at.x+(width-footer_width)*0.5F,at.y+height-13},ink,shortcuts.c_str());
    ImGui::Dummy({width,height});
}
