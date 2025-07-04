#pragma once

#include <SDL3/SDL.h>

enum LoadStates {
    LOAD_NONE,
    LOAD_START,
    LOAD_UNLOAD,
    LOAD_LOAD,
    LOAD_END,
};

struct LoadState {
    enum LoadStates state;

    char level[128];
    uint32_t room;
    uint16_t tag;
};

void init(const char*, const char*);
void loop();
void cleanup();

enum LoadStates get_load_state();
void start_loading(const char*, uint32_t, uint16_t);
