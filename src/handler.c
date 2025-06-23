#include "handler.h"
#include "log.h"
#include "mem.h"
#include "script.h"

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
        lame_free(&handler);
        handler = temp;
    }

    INFO("Closed");
}

int define_handler(lua_State* L) {
    luaL_checkstring(L, -2);
    luaL_checktype(L, -1, LUA_TTABLE);

    struct Handler* handler = lame_alloc(sizeof(struct Handler));
    SDL_strlcpy(handler->name, lua_tostring(L, -2), HANDLER_NAME_MAX);

    // Get virtual functions
    HANDLER_HOOK_FUNCTION(on_register);
    HANDLER_HOOK_FUNCTION(on_start);

    HANDLER_HOOK_FUNCTION(player_activated);
    HANDLER_HOOK_FUNCTION(player_deactivated);

    HANDLER_HOOK_FUNCTION(level_loading);
    HANDLER_HOOK_FUNCTION(level_started);

    HANDLER_HOOK_FUNCTION(room_activated);
    HANDLER_HOOK_FUNCTION(room_deactivated);
    HANDLER_HOOK_FUNCTION(room_changed);

    HANDLER_HOOK_FUNCTION(ui_signalled);

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

    INFO("Defined handler \"%s\"", handler->name);

    // Register handler
    if (handler->on_register != LUA_NOREF)
        execute_ref(handler->on_register, handler->name);

    return 0;
}
