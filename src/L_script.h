#pragma once

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "L_memory.h"

#define SCRIPT_PREFIX(name) s_##name
#define SCRIPT_PUSH(type) lua_push##type
#define SCRIPT_TO(type) lua_to##type

#define EXPOSE_PACKAGE(name, function)                                                                                 \
    luaL_requiref(context, name, function, 1);                                                                         \
    lua_pop(context, 1);

#define EXPOSE_INTEGER(name)                                                                                           \
    lua_pushinteger(context, name);                                                                                    \
    lua_setglobal(context, #name);

#define EXPOSE_NUMBER(name)                                                                                            \
    lua_pushnumber(context, name);                                                                                     \
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
        SCRIPT_PUSH(type)(L, name(luaL_checkinteger(L, 1)));                                                           \
        return 1;                                                                                                      \
    }

#define SCRIPT_GETTER_FLAG(name, type)                                                                                 \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        int slot = luaL_checkinteger(L, 1);                                                                            \
        const char* flag = luaL_checkstring(L, 2);                                                                     \
        SCRIPT_PUSH(type)(L, name(slot, flag));                                                                        \
        return 1;                                                                                                      \
    }

#define SCRIPT_LOG(L, format, ...)                                                                                     \
    do {                                                                                                               \
        lua_Debug* trace = script_stack_trace(L);                                                                      \
        log_script(trace->source, trace->name, trace->currentline, format, ##__VA_ARGS__);                             \
    } while (0);

#define SCRIPT_ASSET(mapname, typename, assettype, hidtype)                                                            \
    SCRIPT_FUNCTION(load_##typename) {                                                                                 \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        load_##typename(name);                                                                                         \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(fetch_##typename) {                                                                                \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        lua_pushinteger(L, fetch_##typename##_hid(name));                                                              \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##typename) {                                                                                  \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        lua_pushinteger(L, get_##typename##_hid(name));                                                                \
        return 1;                                                                                                      \
    }

#define EXPOSE_ASSET(mapname, typename)                                                                                \
    EXPOSE_FUNCTION(load_##typename);                                                                                  \
    EXPOSE_FUNCTION(fetch_##typename);                                                                                 \
    EXPOSE_FUNCTION(get_##typename);

#define execute_buffer(buffer, size, name) _execute_buffer(buffer, size, name, __FILE__, __LINE__)
#define execute_ref(ref, name) _execute_ref(ref, name, __FILE__, __LINE__)
#define execute_ref_in(ref, hid, name) _execute_ref_in(ref, hid, name, __FILE__, __LINE__)

void script_init();
void script_teardown();

void set_import_path(const char*);

void _execute_buffer(void*, size_t, const char*, const char*, int);

int create_table_ref();
int function_ref(int, const char*);
void unreference(int*);
void _execute_ref(int, const char*, const char*, int);
void _execute_ref_in(int, HandleID, const char*, const char*, int);

lua_Debug* script_stack_trace(lua_State*);
