#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual native SDL sensor callbacks with modeled Pad input, no console."""
import argparse,os,pathlib,subprocess,tempfile,tarfile
root=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('sdl',type=pathlib.Path,nargs='?');a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='dkrr-native-motion-') as tmp:
 t=pathlib.Path(tmp);sdl=a.sdl
 if sdl is None:
  sdl=t/'sdl'
  with tarfile.open(root/'vendor/ps5-SDL.tar.gz') as z:z.extractall(sdl,filter='data')
  subprocess.run(['python3',str(root/'tools/apply_sdl_patch.py'),str(sdl)],check=True)
 text=(sdl/'src/joystick/ps5/SDL_ps5joystick.c').read_text()
 def extract(name):
  pos=text.index(name+'(');start=text.rfind('static ',0,pos);begin=text.index('{',pos);end=begin+1;depth=1
  while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
  return text[start:end]
 code=r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "SDL_ps5joystick.h"
typedef uint64_t Uint64;typedef uint32_t Uint32;typedef uint8_t Uint8;typedef int SDL_bool;
#define SDL_TRUE 1
#define SDL_FALSE 0
#define SDL_STANDARD_GRAVITY 9.80665f
#define SDL_SENSOR_GYRO 1
#define SDL_SENSOR_ACCEL 2
#define SDL_HAT_UP 1
#define SDL_HAT_DOWN 2
#define SDL_HAT_LEFT 4
#define SDL_HAT_RIGHT 8
#define SDL_PRESSED 1
#define SDL_RELEASED 0
#define PS5_PAD_AXIS_LX 0
#define PS5_PAD_AXIS_LY 1
#define PS5_PAD_AXIS_RX 2
#define PS5_PAD_AXIS_RY 3
#define PS5_PAD_AXIS_L2 4
#define PS5_PAD_AXIS_R2 5
#define SDL_arraysize(v) ((int)(sizeof(v)/sizeof(v[0])))
#define SDL_zero(v) memset(&(v),0,sizeof(v))
#define SDL_memcpy memcpy
#define SDL_isnan isnan
#define SDL_isinf isinf
typedef struct SDL_Joystick {int unused;} SDL_Joystick;
typedef struct PS5_PadContext {int handle;PS5_PadData pad;SDL_bool sensors_enabled;Uint64 sensor_timestamp;} PS5_PadContext;
static PS5_PadContext context={.handle=17};static PS5_PadData incoming;
static int available=1,returned=0,motion_calls=0,sensor_calls=0,axis_calls=0;static uint64_t stamp;
static float received[2][3];static const unsigned btn_map[]={PS5_PAD_BUTTON_CROSS};static int analog_map[256];
static PS5_PadContext*PS5_JoystickGetPadContext(SDL_Joystick*j){(void)j;return available?&context:NULL;}
static int SDL_SetError(const char*f,...){(void)f;return -1;}
static int SDL_Unsupported(void){return -1;}
int scePadReadState(int h,PS5_PadData*p){assert(h==17);*p=incoming;return 0;}
int scePadSetMotionSensorState(int h,int enabled){assert(h==17&&(enabled==0||enabled==1));++motion_calls;return returned;}
static void SDL_PrivateJoystickAxis(SDL_Joystick*j,int axis,int value){(void)j;(void)axis;(void)value;++axis_calls;}
static void SDL_PrivateJoystickButton(SDL_Joystick*j,int b,int v){(void)j;(void)b;(void)v;}
static void SDL_PrivateJoystickHat(SDL_Joystick*j,int h,int v){(void)j;(void)h;(void)v;}
static void SDL_PrivateJoystickSensor(SDL_Joystick*j,int type,uint64_t ts,const float*p,int n){(void)j;assert(n==3&&(type==1||type==2));++sensor_calls;stamp=ts;memcpy(received[type-1],p,3*sizeof(float));}
'''
 code+=extract('PS5_JoystickSetSensorsEnabled')+'\n'+extract('PS5_JoystickUpdate')
 code+=r'''
int main(void){SDL_Joystick joy={0};
 incoming.connected=1;incoming.timestamp=1000;incoming.vel.x=1.25f;incoming.vel.y=-2.5f;incoming.vel.z=0.75f;incoming.acell.y=1;
 PS5_JoystickUpdate(&joy);assert(!sensor_calls);
 assert(PS5_JoystickSetSensorsEnabled(&joy,1)==0&&motion_calls==1&&context.sensors_enabled);
 PS5_JoystickUpdate(&joy);assert(sensor_calls==2&&stamp==1000&&received[0][0]==1.25f&&received[0][1]==-2.5f&&received[0][2]==0.75f);assert(fabsf(received[1][1]-9.80665f)<0.0001f);
 PS5_JoystickUpdate(&joy);assert(sensor_calls==2);
 incoming.timestamp=2000;incoming.vel.x=NAN;PS5_JoystickUpdate(&joy);assert(sensor_calls==2);
 incoming.vel.x=1;incoming.timestamp=3000;PS5_JoystickUpdate(&joy);assert(sensor_calls==4&&stamp==3000);
 incoming.connected=0;PS5_JoystickUpdate(&joy);assert(sensor_calls==4&&axis_calls>=6);
 incoming.connected=1;assert(PS5_JoystickSetSensorsEnabled(&joy,0)==0&&!context.sensors_enabled);incoming.timestamp=4000;PS5_JoystickUpdate(&joy);assert(sensor_calls==4);
 returned=-9;assert(PS5_JoystickSetSensorsEnabled(&joy,1)==-1&&!context.sensors_enabled);
 available=0;assert(PS5_JoystickSetSensorsEnabled(&joy,1)==-1);
 puts("PASS actual SDL sensor callbacks: native values, g conversion, exact timestamps, duplicate/NaN rejection, disconnect neutralization, disable and API-error handling");}
'''
 (t/'test.c').write_text(code);out=t/'motion'
 subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(sdl/'src/joystick/ps5'),str(t/'test.c'),'-lm','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
