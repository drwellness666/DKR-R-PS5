/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original diagnostic code. No DKR game code, ROM or game artwork.
 * Software video only: a successful run does not validate RT64 or RADV.
 */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef DKR_PROBE_BUILD_LABEL
#define DKR_PROBE_BUILD_LABEL "platform"
#endif

/* Independently authored, minimal 5x7 text glyphs for the diagnostic. */
static const unsigned char glyphs[][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}
};
static void text(SDL_Renderer *r, int x, int y, const char *s, SDL_Color c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,255);
    for (;*s;s++,x+=18) {
        int idx=(*s>='A'&&*s<='Z')?*s-'A':(*s>='0'&&*s<='9')?26+*s-'0':-1;
        for(int row=0;row<7;row++) for(int col=0;col<5;col++) {
            int bit=idx>=0?((glyphs[idx][row]>>(4-col))&1):
                    (*s=='-'&&row==3)||( *s=='.'&&row==6&&col==2)||
                    (*s=='/'&&col==4-row*5/7);
            if(bit) {SDL_Rect box={x+col*3,y+row*3,3,3};SDL_RenderFillRect(r,&box);}
        }
    }
}
static int readable_dir(const char *p) {
    struct stat st;return stat(p,&st)==0&&S_ISDIR(st.st_mode)&&access(p,R_OK)==0;
}
static int rom_header(const char *p) {
    unsigned char b[4];FILE *f=fopen(p,"rb");if(!f)return 0;
    size_t n=fread(b,1,4,f);fclose(f);if(n!=4)return 0;
    return (b[0]==0x80&&b[1]==0x37&&b[2]==0x12&&b[3]==0x40)||
           (b[0]==0x37&&b[1]==0x80&&b[2]==0x40&&b[3]==0x12)||
           (b[0]==0x40&&b[1]==0x12&&b[2]==0x37&&b[3]==0x80);
}
static int tone(SDL_AudioDeviceID dev) {
    /* Quarter-second, quiet 440 Hz sine with short fades; 48 kHz stereo. */
    Sint16 samples[12000*2];
    if(!dev)return -1;
    for(int i=0;i<12000;i++) {
        double fade=i<240?i/240.0:i>=11760?(12000-i)/240.0:1.0;
        Sint16 v=(Sint16)(1600.0*fade*sin(6.283185307179586*440.0*i/48000.0));
        samples[2*i]=samples[2*i+1]=v;
    }
    SDL_ClearQueuedAudio(dev);
    return SDL_QueueAudio(dev,samples,sizeof(samples));
}
int main(int argc,char **argv) {
    printf("probe: build=%s\n",DKR_PROBE_BUILD_LABEL);fflush(stdout);
    SDL_SetMainReady();
    const char *rom=argc>1?argv[1]:"/data/homebrew/DKR-R/roms/dkr.z64";
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS|SDL_INIT_TIMER)<0) {
        fprintf(stderr,"probe: SDL init failed: %s\n",SDL_GetError());SDL_Quit();return 1;
    }
    SDL_Window *win=SDL_CreateWindow("DKR-R PS5 platform probe",0,0,1280,720,SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer *r=win?SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE):NULL;
    if(!r) {fprintf(stderr,"probe: video failed: %s\n",SDL_GetError());if(win)SDL_DestroyWindow(win);SDL_Quit();return 2;}
    SDL_RenderSetLogicalSize(r,1280,720);
    SDL_AudioDeviceID audio=0;SDL_AudioSpec wanted={0};
    wanted.freq=48000;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=512;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)==0)audio=SDL_OpenAudioDevice(NULL,0,&wanted,NULL,0);
    if(audio)SDL_PauseAudioDevice(audio,0);
    SDL_GameController *pads[4]={0};
    for(int i=0,j=0;i<SDL_NumJoysticks()&&j<4;i++)if(SDL_IsGameController(i))pads[j++]=SDL_GameControllerOpen(i);
    SDL_Color white={235,240,250,255},green={70,220,130,255},amber={240,185,70,255};
    int running=1,frames=0,last_tone=0;char line[80];
    int data_ok=readable_dir("/data"),app_ok=readable_dir("/app0"),rom_ok=rom_header(rom);
    printf("probe: video=%s audio=%s /data=%d /app0=%d rom-header=%d\n",
           SDL_GetCurrentVideoDriver(),audio?"opened":"unavailable",data_ok,app_ok,rom_ok);fflush(stdout);
    while(running) {
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT)running=0;
            if(ev.type==SDL_KEYDOWN) {
                if(ev.key.keysym.sym==SDLK_ESCAPE)running=0;
                if(ev.key.keysym.sym==SDLK_SPACE)last_tone=tone(audio);
            }
            if(ev.type==SDL_CONTROLLERBUTTONDOWN) {
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_B)running=0;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_A)last_tone=tone(audio);
            }
            if(ev.type==SDL_CONTROLLERDEVICEADDED) {
                SDL_JoystickID id=SDL_JoystickGetDeviceInstanceID(ev.cdevice.which);
                int known=0;
                for(int j=0;j<4;j++)if(pads[j]&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[j]))==id)known=1;
                if(!known&&SDL_IsGameController(ev.cdevice.which))
                    for(int j=0;j<4;j++)if(!pads[j]) {pads[j]=SDL_GameControllerOpen(ev.cdevice.which);break;}
            }
            if(ev.type==SDL_CONTROLLERDEVICEREMOVED)for(int j=0;j<4;j++)if(pads[j]&&!SDL_GameControllerGetAttached(pads[j])){SDL_GameControllerClose(pads[j]);pads[j]=NULL;}
        }
        SDL_SetRenderDrawColor(r,18,24,38,255);SDL_RenderClear(r);
        text(r,48,42,"DKR-R PS5 PLATFORM PROBE",white);
        text(r,48,90,"DIAGNOSTIC ONLY - NO GAME",amber);
        text(r,48,145,"NATIVE SDL2 SOFTWARE VIDEO",green);
        text(r,48,187,"RT64 AND GPU VULKAN UNTESTED",amber);
        text(r,48,240,audio?"AUDIO OPEN - CROSS PLAYS TONE":"AUDIO UNAVAILABLE",audio?green:amber);
        text(r,48,282,data_ok?"/DATA READABLE":"/DATA NOT READABLE",data_ok?green:amber);
        text(r,48,324,app_ok?"/APP0 READABLE":"/APP0 NOT READABLE",app_ok?green:amber);
        text(r,48,366,rom_ok?"ROM HEADER FOUND - HASH UNCHECKED":"ROM NOT FOUND - OPTIONAL",rom_ok?green:amber);
        for(int j=0;j<4;j++) {
            if(pads[j])snprintf(line,sizeof(line),"PAD %d X %d Y %d",j+1,
              SDL_GameControllerGetAxis(pads[j],SDL_CONTROLLER_AXIS_LEFTX),SDL_GameControllerGetAxis(pads[j],SDL_CONTROLLER_AXIS_LEFTY));
            else snprintf(line,sizeof(line),"PAD %d NOT CONNECTED",j+1);
            text(r,48,414+j*38,line,pads[j]?green:amber);
        }
        text(r,48,588,last_tone<0?"TONE FAILED":"CROSS TONE - CIRCLE EXIT",last_tone<0?amber:white);
        text(r,48,628,"AUTO EXIT AFTER 60 SECONDS",white);
        SDL_Rect bar={48+(frames%1100),676,32,8};SDL_SetRenderDrawColor(r,235,240,250,255);SDL_RenderFillRect(r,&bar);
        SDL_ClearError();SDL_RenderPresent(r);
        if(*SDL_GetError()) {
            fprintf(stderr,"probe: present failed: %s\n",SDL_GetError());running=0;
        } else if(frames==0) {printf("probe: first present returned without SDL error\n");fflush(stdout);}
        SDL_Delay(16);frames++;
        if(SDL_GetTicks()>=60000)running=0;
    }
    if(audio)SDL_CloseAudioDevice(audio);
    for(int j=0;j<4;j++)if(pads[j])SDL_GameControllerClose(pads[j]);
    SDL_DestroyRenderer(r);SDL_DestroyWindow(win);SDL_Quit();
    printf("probe: exited after %d frames\n",frames);fflush(stdout);return 0;
}
