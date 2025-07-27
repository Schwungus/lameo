#pragma once

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "L_file.h" // IWYU pragma: keep

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

#define SCRIPT_FUNCTION_DIRECT(name)                                                                                   \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        name();                                                                                                        \
        return 0;                                                                                                      \
    }

#define SCRIPT_CHECKER(name, type)                                                                                     \
    static type SCRIPT_PREFIX(check_##name)(lua_State * L, int arg) {                                                  \
        void** ptr = (void**)luaL_checkudata(L, arg, #name);                                                           \
        if (*ptr == NULL)                                                                                              \
            luaL_error(L, "Null " #name);                                                                              \
        return (type)(*ptr);                                                                                           \
    }

#define SCRIPT_TESTER(name, type)                                                                                      \
    static type SCRIPT_PREFIX(test_##name)(lua_State * L, int arg) {                                                   \
        void** ptr = (void**)luaL_testudata(L, arg, #name);                                                            \
        return (ptr != NULL && *ptr != NULL) ? (type)(*ptr) : NULL;                                                    \
    }

#define SCRIPT_CHECKER_DIRECT(name, type)                                                                              \
    static type SCRIPT_PREFIX(check_##name)(lua_State * L, int arg) {                                                  \
        type ptr = luaL_checkudata(L, arg, #name);                                                                     \
        if (ptr == NULL)                                                                                               \
            luaL_error(L, "Null " #name);                                                                              \
        return ptr;                                                                                                    \
    }

#define SCRIPT_TESTER_DIRECT(name, type)                                                                               \
    static type SCRIPT_PREFIX(test_##name)(lua_State * L, int arg) {                                                   \
        return (type)luaL_testudata(L, arg, #name);                                                                    \
    }

#define SCRIPT_GETTER(name, type)                                                                                      \
    static int SCRIPT_PREFIX(name)(lua_State * L) {                                                                    \
        SCRIPT_PUSH(type)(L, name());                                                                                  \
        return 1;                                                                                                      \
    }

#define SCRIPT_FLAGS_READ(scope)                                                                                       \
    SCRIPT_FUNCTION(get_##scope##_type) {                                                                              \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        lua_pushinteger(L, get_##scope##_type(name));                                                                  \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##scope##_bool) {                                                                              \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const bool failsafe = luaL_optinteger(L, 2, false);                                                            \
        lua_pushboolean(L, get_##scope##_bool(name, failsafe));                                                        \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##scope##_int) {                                                                               \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const int failsafe = (int)luaL_optinteger(L, 2, 0);                                                            \
        lua_pushinteger(L, get_##scope##_int(name, failsafe));                                                         \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##scope##_float) {                                                                             \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const float failsafe = (float)luaL_optnumber(L, 2, 0);                                                         \
        lua_pushnumber(L, get_##scope##_float(name, failsafe));                                                        \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##scope##_string) {                                                                            \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const char* fallback = luaL_optstring(L, 2, NULL);                                                             \
        const char* str = get_##scope##_string(name, fallback);                                                        \
                                                                                                                       \
        if (str == NULL)                                                                                               \
            lua_pushnil(L);                                                                                            \
        else                                                                                                           \
            lua_pushstring(L, str);                                                                                    \
        return 1;                                                                                                      \
    }

#define SCRIPT_FLAGS_WRITE(scope)                                                                                      \
    SCRIPT_FUNCTION_DIRECT(clear_##scope);                                                                             \
                                                                                                                       \
    SCRIPT_FUNCTION(delete_##scope) {                                                                                  \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        delete_##scope(name);                                                                                          \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(set_##scope##_bool) {                                                                              \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const bool value = luaL_checkinteger(L, 2);                                                                    \
        set_##scope##_bool(name, value);                                                                               \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(set_##scope##_int) {                                                                               \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const int value = luaL_checkinteger(L, 2);                                                                     \
        set_##scope##_int(name, value);                                                                                \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(set_##scope##_float) {                                                                             \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const float value = luaL_checknumber(L, 2);                                                                    \
        set_##scope##_float(name, value);                                                                              \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(set_##scope##_string) {                                                                            \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const char* value = luaL_checkstring(L, 2);                                                                    \
        set_##scope##_string(name, value);                                                                             \
        return 0;                                                                                                      \
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
        const assettype asset = fetch_##typename(name);                                                                \
        if (asset == NULL)                                                                                             \
            lua_pushnil(L);                                                                                            \
        else                                                                                                           \
            lua_rawgeti(L, LUA_REGISTRYINDEX, asset->userdata);                                                        \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_FUNCTION(get_##typename) {                                                                                  \
        const char* name = luaL_checkstring(L, 1);                                                                     \
        const assettype asset = get_##typename(name);                                                                  \
        if (asset == NULL)                                                                                             \
            lua_pushnil(L);                                                                                            \
        else                                                                                                           \
            lua_rawgeti(L, LUA_REGISTRYINDEX, asset->userdata);                                                        \
        return 1;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    SCRIPT_CHECKER(typename, assettype);                                                                               \
    SCRIPT_TESTER(typename, assettype);

#define EXPOSE_ASSET(mapname, typename)                                                                                \
    luaL_newmetatable(context, #typename);                                                                             \
    lua_pop(context, 1);                                                                                               \
    EXPOSE_FUNCTION(load_##typename);                                                                                  \
    EXPOSE_FUNCTION(fetch_##typename);                                                                                 \
    EXPOSE_FUNCTION(get_##typename);

#define EXPOSE_FLAGS_READ(scope)                                                                                       \
    EXPOSE_FUNCTION(get_##scope##_type);                                                                               \
    EXPOSE_FUNCTION(get_##scope##_bool);                                                                               \
    EXPOSE_FUNCTION(get_##scope##_int);                                                                                \
    EXPOSE_FUNCTION(get_##scope##_float);                                                                              \
    EXPOSE_FUNCTION(get_##scope##_string);

#define EXPOSE_FLAGS_WRITE(scope)                                                                                      \
    EXPOSE_FUNCTION(clear_##scope);                                                                                    \
    EXPOSE_FUNCTION(delete_##scope);                                                                                   \
    EXPOSE_FUNCTION(set_##scope##_bool);                                                                               \
    EXPOSE_FUNCTION(set_##scope##_int);                                                                                \
    EXPOSE_FUNCTION(set_##scope##_float);                                                                              \
    EXPOSE_FUNCTION(set_##scope##_string);

#define execute_buffer(buffer, size, name) _execute_buffer(buffer, size, name, __FILE__, __LINE__)
#define execute_ref(ref, name) _execute_ref(ref, name, __FILE__, __LINE__)
#define execute_ref_in(ref, userdata, name) _execute_ref_in(ref, userdata, name, __FILE__, __LINE__)
#define execute_ref_in_child(ref, userdata, userdata2, name)                                                           \
    _execute_ref_in_child(ref, userdata, userdata2, name, __FILE__, __LINE__)

void script_init();
void script_teardown();

void set_import_path(const char*);

void _execute_buffer(void*, size_t, const char*, const char*, int);

void collect_garbage();
void* userdata_alloc(const char*, size_t);
void* _userdata_alloc_clean(const char*, size_t, const char*, int);

#define userdata_alloc_clean(type, size) _userdata_alloc_clean(type, size, __FILE__, __LINE__)

int create_ref();
int create_table_ref();
int create_pointer_ref(const char*, void*);
int function_ref(int, const char*);
int json_to_table_ref(yyjson_val*);
void copy_table(int, int);

void unreference(int*);
void unreference_pointer(int*);

void _execute_ref(int, const char*, const char*, int);
void _execute_ref_in(int, int, const char*, const char*, int);
void _execute_ref_in_child(int, int, int, const char*, const char*, int);

lua_Debug* script_stack_trace(lua_State*);
