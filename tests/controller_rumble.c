/* SPDX-License-Identifier: GPL-3.0-or-later
 * Run the actual PS5 SDL rumble function with a modeled ScePad transport.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint16_t Uint16;
typedef struct {int unused;} SDL_Joystick;
typedef struct {int handle;} PS5_PadContext;
typedef struct {uint8_t largeMotor,smallMotor;} PS5_PadVibration;
static PS5_PadContext context={17};
static int connected=1,calls=0,return_code=0,last_handle=-1;
static PS5_PadVibration last;
static PS5_PadContext *PS5_JoystickGetPadContext(SDL_Joystick *j){(void)j;return connected?&context:NULL;}
static int SDL_SetError(const char *format,...){(void)format;return -1;}
static int scePadSetVibration(int handle,const PS5_PadVibration *v){++calls;last_handle=handle;last=*v;return return_code;}
#include "ps5_rumble.inc"
int main(void) {
 SDL_Joystick pad={0};
 assert(!PS5_JoystickRumble(&pad,65535,0));assert(calls==1&&last_handle==17&&last.largeMotor==255&&last.smallMotor==0);
 assert(!PS5_JoystickRumble(&pad,32767,65535));assert(last.largeMotor==127&&last.smallMotor==255);
 assert(!PS5_JoystickRumble(&pad,0,0));assert(last.largeMotor==0&&last.smallMotor==0);
 int old=calls;connected=0;assert(PS5_JoystickRumble(&pad,65535,65535)<0&&calls==old);
 connected=1;return_code=-23;assert(PS5_JoystickRumble(&pad,1,256)<0&&calls==old+1);
 puts("PASS: actual PS5 SDL rumble scaling, stop, disconnected device and ScePad error propagation (modeled transport)");
}
