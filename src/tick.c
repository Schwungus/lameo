#include <SDL3/SDL.h>

#include "internal.h"
#include "tick.h"

static uint64_t last_time = 0;
static float ticks = 0;

void tick_init() {
    last_time = SDL_GetTicks();

    INFO("Opened");
}

void tick_update() {
    const uint64_t current_time = SDL_GetTicks();
    ticks += (float)(current_time - last_time) * ((float)TICKRATE / 1000.);

    if (ticks >= 1) {
        if (get_load_state() == LOAD_NONE)
            while (ticks >= 1) {
                // UI, game ...
                ticks -= 1;
            }
        else
            ticks -= SDL_floorf(ticks);
    }

    last_time = current_time;
}

void tick_teardown() {
    INFO("Closed");
}
