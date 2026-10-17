// SPDX-License-Identifier: GPL-3.0-or-later
#version 450
#extension GL_EXT_samplerless_texture_functions : require
layout(set=0,binding=0) uniform texture2D sceneImage;
layout(set=0,binding=1) uniform texture2D uiImage;
layout(push_constant) uniform Calibration {vec4 values;} cfg;
layout(location=0) out vec4 color;
vec3 linear709(vec3 x){x=clamp(x,0,1);return mix(pow((x+0.055)/1.055,vec3(2.4)),x/12.92,lessThanEqual(x,vec3(0.04045)));}
vec3 rec2020(vec3 x){return vec3(dot(x,vec3(0.627404,0.329283,0.043313)),dot(x,vec3(0.069097,0.919540,0.011362)),dot(x,vec3(0.016391,0.088013,0.895595)));}
vec3 pq(vec3 n){vec3 v=pow(clamp(n/10000.0,0,1),vec3(2610.0/16384.0));vec3 r=pow((3424.0/4096.0+(2413.0/128.0)*v)/(1+(2392.0/128.0)*v),vec3(2523.0/32.0));return mix(r,vec3(0),lessThanEqual(n,vec3(0)));}
void main(){
    ivec2 p=ivec2(gl_FragCoord.xy);vec3 scene=linear709(texelFetch(sceneImage,p,0).rgb);
    vec4 ui=texelFetch(uiImage,p,0);float a=clamp(ui.a,0,1);
    vec3 overlay=linear709(a>0.000001?ui.rgb/a:vec3(0));
    float l=dot(scene,vec3(0.2126,0.7152,0.0722));
    float k=smoothstep(0.7,1,l);
    float scale=cfg.values.x+(cfg.values.y-cfg.values.x)*cfg.values.z*k;
    vec3 n=rec2020(scene)*scale*(1-a)+rec2020(overlay)*cfg.values.x*a;
    if(cfg.values.w>0.5){
        ivec2 size=textureSize(sceneImage,0);
        if(p.y>size.y*3/4){bool checker=((p.x/32+p.y/32)%2)==0;
            n=vec3(p.x<size.x/2?(checker?cfg.values.x:0):(checker?cfg.values.y:cfg.values.y*0.98));}
    }
    color=vec4(pq(clamp(n,vec3(0),vec3(cfg.values.y))),1);
}
