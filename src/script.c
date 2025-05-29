#include <SDL3/SDL_iostream.h>
#include <lauxlib.h>
#include <lualib.h>

#include "flags.h"
#include "log.h"
#include "mem.h"
#include "mod.h"
#include "player.h"
#include "script.h"

static lua_State* context = NULL;
static lua_Debug debug = {0};

// Script functions
// Types
SCRIPT_FUNCTION(tostring) {
    luaL_tolstring(L, -1, NULL);
    return 1;
}

// Debug
SCRIPT_FUNCTION(print) {
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nSl", &debug);
    const char* str = lua_tostring(L, -1);
    log_script(debug.source, debug.name, debug.currentline, str == NULL ? "(null)" : str);
    return 0;
}

SCRIPT_FUNCTION(error) {
    luaL_error(L, lua_tostring(L, -1));
    return 0;
}

// Flags
SCRIPT_FUNCTION(get_flag_type) {
    switch (get_flag_type(lua_tointeger(L, -2), lua_tostring(L, -1))) {
        default:
        case FT_NULL:
            lua_pushstring(L, "nil");
            break;
        case FT_BOOL:
            lua_pushstring(L, "boolean");
            break;
        case FT_INT:
        case FT_FLOAT:
            lua_pushstring(L, "number");
            break;
        case FT_STRING:
            lua_pushstring(L, "string");
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(get_flag) {
    enum FlagScope scope = lua_tointeger(L, -2);
    const char* flag = lua_tostring(L, -1);

    switch (get_flag_type(scope, flag)) {
        default:
        case FT_NULL:
            lua_pushnil(L);
            break;
        case FT_BOOL:
            lua_pushboolean(L, get_bool_flag(scope, flag, false));
            break;
        case FT_INT:
            lua_pushinteger(L, get_int_flag(scope, flag, 0));
            break;
        case FT_FLOAT:
            lua_pushnumber(L, get_float_flag(scope, flag, 0));
            break;
        case FT_STRING:
            lua_pushstring(L, get_string_flag(scope, flag, NULL));
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(set_flag) {
    enum FlagScope scope = lua_tointeger(L, -3);
    const char* flag = lua_tostring(L, -2);

    switch (lua_type(L, -1)) {
        default:
        case LUA_TNIL:
            reset_flag(scope, flag);
            break;
        case LUA_TBOOLEAN:
            set_bool_flag(scope, flag, lua_toboolean(L, -1));
            break;
        case LUA_TNUMBER: {
            if (lua_isinteger(L, -1))
                set_int_flag(scope, flag, lua_tointeger(L, -1));
            else
                set_float_flag(scope, flag, lua_tonumber(L, -1));
            break;
        }
        case LUA_TSTRING:
            set_string_flag(scope, flag, lua_tostring(L, -1));
            break;
    }

    return 0;
}

SCRIPT_GETTER_FLAG(toggle_flag, boolean);
SCRIPT_GETTER_FLAG(increment_flag, integer);
SCRIPT_GETTER_FLAG(decrement_flag, integer);

// Players
SCRIPT_GETTER_SLOT(next_ready_player, integer);
SCRIPT_GETTER_SLOT(next_active_player, integer);

SCRIPT_FUNCTION(get_pflag_type) {
    switch (get_pflag_type(lua_tointeger(L, -2), lua_tostring(L, -1))) {
        default:
        case FT_NULL:
            lua_pushstring(L, "nil");
            break;
        case FT_BOOL:
            lua_pushstring(L, "boolean");
            break;
        case FT_INT:
        case FT_FLOAT:
            lua_pushstring(L, "number");
            break;
        case FT_STRING:
            lua_pushstring(L, "string");
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(get_pflag) {
    int slot = lua_tointeger(L, -2);
    const char* flag = lua_tostring(L, -1);

    switch (get_pflag_type(slot, flag)) {
        default:
        case FT_NULL:
            lua_pushnil(L);
            break;
        case FT_BOOL:
            lua_pushboolean(L, get_bool_pflag(slot, flag, false));
            break;
        case FT_INT:
            lua_pushinteger(L, get_int_pflag(slot, flag, 0));
            break;
        case FT_FLOAT:
            lua_pushnumber(L, get_float_pflag(slot, flag, 0));
            break;
        case FT_STRING:
            lua_pushstring(L, get_string_pflag(slot, flag, NULL));
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(set_pflag) {
    int slot = lua_tointeger(L, -3);
    const char* flag = lua_tostring(L, -2);

    switch (lua_type(L, -1)) {
        default:
        case LUA_TNIL:
            reset_pflag(slot, flag);
            break;
        case LUA_TBOOLEAN:
            set_bool_pflag(slot, flag, lua_toboolean(L, -1));
            break;
        case LUA_TNUMBER: {
            if (lua_isinteger(L, -1))
                set_int_pflag(slot, flag, lua_tointeger(L, -1));
            else
                set_float_pflag(slot, flag, lua_tonumber(L, -1));
            break;
        }
        case LUA_TSTRING:
            set_string_pflag(slot, flag, lua_tostring(L, -1));
            break;
    }

    return 0;
}

SCRIPT_GETTER_FLAG(toggle_pflag, boolean);
SCRIPT_GETTER_FLAG(increment_pflag, integer);
SCRIPT_GETTER_FLAG(decrement_pflag, integer);

// Meat and bones
void* script_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize <= 0) {
        if (ptr != NULL)
            lame_free(&ptr);
        return NULL;
    }

    if (ptr == NULL)
        return lame_alloc(nsize);

    lame_realloc(&ptr, nsize);
    return ptr;
}

void script_init() {
    context = lua_newstate(script_alloc, NULL);

    // Expose a lot of things
    EXPOSE_PACKAGE(LUA_LOADLIBNAME, luaopen_package);
    EXPOSE_PACKAGE(LUA_MATHLIBNAME, luaopen_math);
    EXPOSE_PACKAGE(LUA_STRLIBNAME, luaopen_string);

    // Types
    EXPOSE_FUNCTION(tostring);

    // Debug
    EXPOSE_FUNCTION(print);
    EXPOSE_FUNCTION(error);

    // Flags
    EXPOSE_INTEGER(FS_STATIC);
    EXPOSE_INTEGER(FS_GLOBAL);
    EXPOSE_INTEGER(FS_LOCAL);

    EXPOSE_FUNCTION(get_flag_type);
    EXPOSE_FUNCTION(get_flag);
    EXPOSE_FUNCTION(set_flag);
    EXPOSE_FUNCTION(toggle_flag);
    EXPOSE_FUNCTION(increment_flag);
    EXPOSE_FUNCTION(decrement_flag);

    // Players
    EXPOSE_INTEGER(PS_INACTIVE);
    EXPOSE_INTEGER(PS_READY);
    EXPOSE_INTEGER(PS_ACTIVE);

    EXPOSE_INTEGER(PB_NONE);
    EXPOSE_INTEGER(PB_JUMP);
    EXPOSE_INTEGER(PB_INTERACT);
    EXPOSE_INTEGER(PB_ATTACK);
    EXPOSE_INTEGER(PB_INVENTORY1);
    EXPOSE_INTEGER(PB_INVENTORY2);
    EXPOSE_INTEGER(PB_INVENTORY3);
    EXPOSE_INTEGER(PB_INVENTORY4);
    EXPOSE_INTEGER(PB_AIM);

    EXPOSE_FUNCTION(next_ready_player);
    EXPOSE_FUNCTION(next_active_player);

    EXPOSE_FUNCTION(get_pflag_type);
    EXPOSE_FUNCTION(get_pflag);
    EXPOSE_FUNCTION(set_pflag);
    EXPOSE_FUNCTION(toggle_pflag);
    EXPOSE_FUNCTION(increment_pflag);
    EXPOSE_FUNCTION(decrement_pflag);

    mod_init_script();

    INFO("Opened");
}

void script_teardown() {
    CLOSE_HANDLE(context, lua_close);

    INFO("Closed");
}

void _execute_buffer(void* buffer, size_t size, const char* name, const char* filename, int line) {
    if (luaL_loadbuffer(context, buffer, size, name))
        log_fatal(src_basename(filename), line, "Failed to load script \"%s\": %s", name, lua_tostring(context, -1));
    if (lua_pcall(context, 0, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error in script \"%s\": %s", name, lua_tostring(context, -1));
}
