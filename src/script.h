#pragma once

#include <lua.h>

#define SCRIPT_PREFIX(name) s_##name
#define SCRIPT_PUSH(type) lua_push##type
#define SCRIPT_TO(type) lua_to##type

#define EXPOSE_PACKAGE(name, function)                                                                                 \
    luaL_requiref(context, name, function, 1);                                                                         \
    lua_pop(context, 1);

#define EXPOSE_INTEGER(name)                                                                                           \
    lua_pushinteger(context, name);                                                                                    \
    lua_setglobal(context, #name);

#define EXPOSE_FUNCTION(name)                                                                                          \
    lua_pushcfunction(context, SCRIPT_PREFIX(name));                                                                   \
    lua_setglobal(context, #name);

#define SCRIPT_FUNCTION(name) static int SCRIPT_PREFIX(name)(lua_State * L)

#define SCRIPT_GETTER(name, type)                                                                                      \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        SCRIPT_PUSH(type)(L, name());                                                                                  \
        return 1;                                                                                                      \
    }

#define SCRIPT_GETTER_SLOT(name, type)                                                                                 \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        SCRIPT_PUSH(type)(L, name(lua_tointeger(L, -1)));                                                              \
        return 1;                                                                                                      \
    }

#define SCRIPT_GETTER_FLAG(name, type)                                                                                 \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        SCRIPT_PUSH(type)(L, name(lua_tointeger(L, -2), lua_tostring(L, -1)));                                         \
        return 1;                                                                                                      \
    }

#define execute_buffer(buffer, size, name) _execute_buffer(buffer, size, name, __FILE__, __LINE__)

void script_init();
void script_teardown();

void _execute_buffer(void*, size_t, const char*, const char*, int);
