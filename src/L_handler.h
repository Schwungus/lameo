#pragma once

#include "L_script.h" // IWYU pragma: keep

#define HANDLER_NAME_MAX 128

struct Handler {
    const char* name;
    struct Handler *previous, *next;

    int on_register, on_start;
    int player_activated, player_deactivated;
    int level_loading, level_started;
    int room_activated, room_deactivated, room_changed;
    int ui_signalled;
};

void handler_init();
void handler_teardown();

int define_handler(lua_State*);
