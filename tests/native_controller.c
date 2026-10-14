/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ps5_controller_mapping.h"
int main(void) {
    assert(SDL_Init(SDL_INIT_GAMECONTROLLER)==0);
    assert(SDL_GameControllerAddMapping(dkrr_ps5_controller_mapping)>=0);
    SDL_JoystickGUID guid=SDL_JoystickGetGUIDFromString("0300d0424c050000e60c000011810000");
    char *mapping=SDL_GameControllerMappingForGUID(guid);
    assert(mapping && strstr(mapping,"back:b4"));SDL_free(mapping);
    int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,17,1);assert(device>=0);
    SDL_Joystick *stick=SDL_JoystickOpen(device);assert(stick);
    char virtual_guid[33],virtual_map[1024];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(stick),virtual_guid,sizeof(virtual_guid));
    snprintf(virtual_map,sizeof(virtual_map),"%s%s",virtual_guid,dkrr_ps5_controller_mapping+32);
    assert(SDL_GameControllerAddMapping(virtual_map)>=0);
    SDL_GameController *pad=SDL_GameControllerOpen(device);assert(pad);
    assert(SDL_JoystickSetVirtualButton(stick,4,SDL_PRESSED)==0);SDL_JoystickUpdate();
    assert(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_BACK)==SDL_PRESSED);
    assert(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_A)==SDL_RELEASED);
    assert(SDL_JoystickSetVirtualButton(stick,4,SDL_RELEASED)==0);
    assert(SDL_JoystickSetVirtualButton(stick,0,SDL_PRESSED)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,0,-20000)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,1,20000)==0);SDL_JoystickUpdate();
    assert(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_BACK)==SDL_RELEASED);
    assert(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_A)==SDL_PRESSED);
    assert(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX)==-20000);
    assert(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY)==20000);
    SDL_GameControllerClose(pad);SDL_JoystickClose(stick);SDL_JoystickDetachVirtual(device);SDL_Quit();
    puts("PASS SDL mapping: touchpad/Back, Cross/A and independent stick axes (virtual host device)");
}
