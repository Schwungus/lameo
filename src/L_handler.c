#include "L_handler.h"
#include "L_log.h"
#include "L_memory.h"

static struct Handler* handlers = NULL;

void handler_init() {
    struct Handler* handler = handlers;
    while (handler != NULL) {
        if (handler->on_start != LUA_NOREF)
            execute_ref(handler->on_start, handler->name);
        handler = handler->next;
    }

    INFO("Opened");
}

void handler_teardown() {
    struct Handler* handler = handlers;
    while (handler != NULL) {
        unreference(&handler->on_register);
        unreference(&handler->on_start);

        unreference(&handler->player_activated);
        unreference(&handler->player_deactivated);

        unreference(&handler->level_loading);
        unreference(&handler->level_started);

        unreference(&handler->room_activated);
        unreference(&handler->room_deactivated);
        unreference(&handler->room_changed);

        unreference(&handler->ui_signalled);

        struct Handler* temp = handler->next;
        lame_free(&handler->name);
        lame_free(&handler);
        handler = temp;
    }

    INFO("Closed");
}

int define_handler(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    struct Handler* handler = lame_alloc(sizeof(struct Handler));
    handler->name = SDL_strdup(name);

    // Get virtual functions
    handler->on_register = function_ref(-1, "on_register");
    handler->on_start = function_ref(-1, "on_start");

    handler->player_activated = function_ref(-1, "player_activated");
    handler->player_deactivated = function_ref(-1, "player_deactivated");

    handler->level_loading = function_ref(-1, "level_loading");
    handler->level_started = function_ref(-1, "level_started");

    handler->room_activated = function_ref(-1, "room_activated");
    handler->room_deactivated = function_ref(-1, "room_deactivated");
    handler->room_changed = function_ref(-1, "room_changed");

    handler->ui_signalled = function_ref(-1, "ui_signalled");

    // Link handler
    if (handlers == NULL) {
        handler->previous = NULL;
        handlers = handler;
    } else {
        struct Handler* it = handlers;
        while (it->next != NULL)
            it = it->next;
        it->next = handler;
        handler->previous = it;
    }
    handler->next = NULL;

    INFO("Defined handler \"%s\"", name);

    // Register handler
    if (handler->on_register != LUA_NOREF)
        execute_ref(handler->on_register, name);

    return 0;
}
