#pragma once

#include "level.h"

enum LoadStates {
    LOAD_NONE,
    LOAD_START,
    LOAD_UNLOAD,
    LOAD_LOAD,
    LOAD_END,
};

struct LoadState {
    enum LoadStates state;

    char level[LEVEL_NAME_MAX];
    uint16_t room;
    uint16_t tag;
};

void init(const char*, const char*);
void loop();
void cleanup();

enum LoadStates get_load_state();
