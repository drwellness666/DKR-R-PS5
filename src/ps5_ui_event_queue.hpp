// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL.h>
#include <mutex>
#include <vector>
namespace dkrr {
// Only events with inline storage that ImGui consumes cross the thread
// boundary. Sensor/axis packets remain available to gameplay via SDL state;
// they must never make the event owner wait for an entire rendered UI frame.
class UiEventQueue {
public:
    struct Batch {std::vector<SDL_Event> events;bool reset_inputs=false;};
    bool push(const SDL_Event& event){
        switch(event.type){
        case SDL_MOUSEMOTION:case SDL_MOUSEWHEEL:case SDL_MOUSEBUTTONDOWN:case SDL_MOUSEBUTTONUP:
        case SDL_TEXTINPUT:case SDL_KEYDOWN:case SDL_KEYUP:case SDL_WINDOWEVENT:
        case SDL_CONTROLLERDEVICEADDED:case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERBUTTONDOWN:case SDL_CONTROLLERBUTTONUP:break;
        default:return false;
        }
        std::lock_guard lock(mutex_);
        if(pending_.events.size()>=512){pending_.events.clear();pending_.reset_inputs=true;}
        pending_.events.push_back(event);return true;
    }
    Batch take(){std::lock_guard lock(mutex_);Batch out;out.events.swap(pending_.events);out.reset_inputs=pending_.reset_inputs;pending_.reset_inputs=false;return out;}
    void clear(){std::lock_guard lock(mutex_);pending_.events.clear();pending_.reset_inputs=false;}
private:
    std::mutex mutex_;Batch pending_;
};
}
