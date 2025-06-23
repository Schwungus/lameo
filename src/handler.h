#pragma once

#include "script.h"

#define HANDLER_NAME_MAX 32

#define HANDLER_HOOK_FUNCTION(funcname)                                                                                \
    lua_getfield(L, -1, #funcname);                                                                                    \
    if (lua_isfunction(L, -1)) {                                                                                       \
        handler->funcname = luaL_ref(L, LUA_REGISTRYINDEX);                                                            \
    } else {                                                                                                           \
        handler->funcname = LUA_NOREF;                                                                                 \
        lua_pop(L, 1);                                                                                                 \
    }

struct Handler {
    char name[HANDLER_NAME_MAX];
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
