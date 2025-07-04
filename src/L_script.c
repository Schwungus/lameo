#include "L_script.h"
#include "L_flags.h"
#include "L_handler.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_mod.h"
#include "L_player.h"
#include "L_ui.h"
#include "L_video.h"

static lua_State* context = NULL;
static lua_Debug debug = {0};

// Script functions
// Types
SCRIPT_FUNCTION(tostring) {
    luaL_tolstring(L, -1, NULL);
    return 1;
}

// Definitions
SCRIPT_FUNCTION(define_handler) {
    return define_handler(L);
}

SCRIPT_FUNCTION(define_ui) {
    return define_ui(L);
}

SCRIPT_FUNCTION(define_actor) {
    return 0;
}

// Debug
SCRIPT_FUNCTION(print) {
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nSl", &debug);
    log_script(debug.source, debug.name, debug.currentline, "%s", lua_tostring(L, -1));
    return 0;
}

SCRIPT_FUNCTION(error) {
    return luaL_error(L, lua_tostring(L, -1));
}

// Flags
SCRIPT_FUNCTION(get_flag_type) {
    enum FlagScopes scope = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

    switch (get_flag_type(scope, flag)) {
        default:
        case FT_NULL:
            lua_pushstring(L, lua_typename(L, LUA_TNIL));
            break;
        case FT_BOOL:
            lua_pushstring(L, lua_typename(L, LUA_TBOOLEAN));
            break;
        case FT_INT:
        case FT_FLOAT:
            lua_pushstring(L, lua_typename(L, LUA_TNUMBER));
            break;
        case FT_STRING:
            lua_pushstring(L, lua_typename(L, LUA_TSTRING));
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(get_flag) {
    enum FlagScopes scope = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

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
    enum FlagScopes scope = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

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
SCRIPT_FUNCTION(get_player_status) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    lua_pushinteger(L, player == NULL ? PS_INACTIVE : player->status);
    return 1;
}

SCRIPT_FUNCTION(get_player_move) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    if (player == NULL) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    } else {
        lua_pushinteger(L, player->input.move[0]);
        lua_pushinteger(L, player->input.move[1]);
    }
    return 2;
}

SCRIPT_FUNCTION(get_player_aim) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    if (player == NULL) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    } else {
        lua_pushinteger(L, player->input.aim[0]);
        lua_pushinteger(L, player->input.aim[1]);
    }
    return 2;
}

SCRIPT_FUNCTION(get_player_buttons) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    lua_pushinteger(L, player == NULL ? PB_NONE : player->input.buttons);
    return 1;
}

SCRIPT_FUNCTION(get_player_last_move) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    if (player == NULL) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    } else {
        lua_pushinteger(L, player->last_input.move[0]);
        lua_pushinteger(L, player->last_input.move[1]);
    }
    return 2;
}

SCRIPT_FUNCTION(get_player_last_aim) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    if (player == NULL) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    } else {
        lua_pushinteger(L, player->last_input.aim[0]);
        lua_pushinteger(L, player->last_input.aim[1]);
    }
    return 2;
}

SCRIPT_FUNCTION(get_player_last_buttons) {
    struct Player* player = get_player(luaL_checkinteger(L, 1));
    lua_pushinteger(L, player == NULL ? PB_NONE : player->last_input.buttons);
    return 1;
}

SCRIPT_GETTER_SLOT(next_ready_player, integer);
SCRIPT_GETTER_SLOT(next_active_player, integer);
SCRIPT_GETTER_SLOT(next_neighbor_player, integer);

SCRIPT_FUNCTION(get_pflag_type) {
    int slot = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

    switch (get_pflag_type(slot, flag)) {
        default:
        case FT_NULL:
            lua_pushstring(L, lua_typename(L, LUA_TNIL));
            break;
        case FT_BOOL:
            lua_pushstring(L, lua_typename(L, LUA_TBOOLEAN));
            break;
        case FT_INT:
        case FT_FLOAT:
            lua_pushstring(L, lua_typename(L, LUA_TNUMBER));
            break;
        case FT_STRING:
            lua_pushstring(L, lua_typename(L, LUA_TSTRING));
            break;
    }

    return 1;
}

SCRIPT_FUNCTION(get_pflag) {
    int slot = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

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
    int slot = luaL_checkinteger(L, 1);
    const char* flag = luaL_checkstring(L, 2);

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

// Localization
SCRIPT_FUNCTION(localized) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushstring(L, localized(key));
    return 1;
}

// Video
SCRIPT_GETTER(get_draw_time, integer);

SCRIPT_FUNCTION(main_rectangle) {
    set_main_texture(NULL);

    const GLfloat x1 = luaL_checknumber(L, 1);
    const GLfloat y1 = luaL_checknumber(L, 2);
    const GLfloat x2 = luaL_checknumber(L, 3);
    const GLfloat y2 = luaL_checknumber(L, 4);
    const GLfloat z = luaL_checknumber(L, 5);
    const GLfloat r = luaL_checkinteger(L, 6);
    const GLfloat g = luaL_checkinteger(L, 7);
    const GLfloat b = luaL_checkinteger(L, 8);
    const GLfloat a = luaL_checkinteger(L, 9);

    main_vertex(x1, y2, z, r, g, b, a, 0, 1);
    main_vertex(x2, y2, z, r, g, b, a, 1, 1);
    main_vertex(x2, y1, z, r, g, b, a, 1, 0);
    main_vertex(x1, y2, z, r, g, b, a, 0, 1);
    main_vertex(x2, y1, z, r, g, b, a, 1, 0);
    main_vertex(x1, y1, z, r, g, b, a, 0, 0);

    return 0;
}

SCRIPT_FUNCTION(main_string) {
    const char* str = luaL_checkstring(L, 1);
    const FontID font = luaL_checknumber(L, 2);
    const GLfloat size = luaL_checknumber(L, 3);
    const GLfloat x = luaL_checknumber(L, 4);
    const GLfloat y = luaL_checknumber(L, 5);
    const GLfloat z = luaL_checknumber(L, 6);

    main_string(str, NULL, size, x, y, z);

    return 0;
}

// UI
SCRIPT_FUNCTION(create_ui) {
    struct UI* parent = hid_to_ui(luaL_checkinteger(L, 1));
    const char* name = luaL_checkstring(L, 2);

    struct UI* ui = create_ui(parent, name);
    lua_pushinteger(L, ui == NULL ? 0 : ui->hid);

    return 1;
}

SCRIPT_FUNCTION(destroy_ui) {
    struct UI* ui = hid_to_ui(luaL_checkinteger(L, 1));
    if (ui != NULL)
        destroy_ui(ui);
    return 0;
}

SCRIPT_FUNCTION(ui_table) {
    struct UI* ui = hid_to_ui(luaL_checkinteger(L, 1));
    if (ui == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, ui->table);
    return 1;
}

SCRIPT_FUNCTION(get_ui_cursor) {
    const vec2* cursor = get_ui_cursor();
    lua_pushnumber(L, (*cursor)[0]);
    lua_pushnumber(L, (*cursor)[1]);
    return 2;
}

SCRIPT_FUNCTION(get_ui_button) {
    int buttons = luaL_checkinteger(L, 1);
    lua_pushboolean(L, get_ui_button(buttons));
    return 1;
}

// Meat and bones
void* script_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize <= 0) {
        FREE_POINTER(ptr);
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

    // Info
    EXPOSE_FUNCTION(define_handler);
    EXPOSE_FUNCTION(define_actor);
    EXPOSE_FUNCTION(define_ui);

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
    EXPOSE_INTEGER(MAX_PLAYERS);

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

    EXPOSE_FUNCTION(get_player_status);
    EXPOSE_FUNCTION(get_player_move);
    EXPOSE_FUNCTION(get_player_aim);
    EXPOSE_FUNCTION(get_player_buttons);
    EXPOSE_FUNCTION(get_player_last_move);
    EXPOSE_FUNCTION(get_player_last_aim);
    EXPOSE_FUNCTION(get_player_last_buttons);

    EXPOSE_FUNCTION(next_ready_player);
    EXPOSE_FUNCTION(next_active_player);
    EXPOSE_FUNCTION(next_neighbor_player);

    EXPOSE_FUNCTION(get_pflag_type);
    EXPOSE_FUNCTION(get_pflag);
    EXPOSE_FUNCTION(set_pflag);
    EXPOSE_FUNCTION(toggle_pflag);
    EXPOSE_FUNCTION(increment_pflag);
    EXPOSE_FUNCTION(decrement_pflag);

    // Localize
    EXPOSE_FUNCTION(localized);

    // Video
    EXPOSE_INTEGER(DEFAULT_DISPLAY_WIDTH);
    EXPOSE_INTEGER(DEFAULT_DISPLAY_HEIGHT);
    EXPOSE_NUMBER(UI_Z);

    EXPOSE_FUNCTION(get_draw_time);

    EXPOSE_FUNCTION(main_rectangle);
    EXPOSE_FUNCTION(main_string);

    // UI
    EXPOSE_INTEGER(UII_UP);
    EXPOSE_INTEGER(UII_LEFT);
    EXPOSE_INTEGER(UII_DOWN);
    EXPOSE_INTEGER(UII_RIGHT);
    EXPOSE_INTEGER(UII_ENTER);
    EXPOSE_INTEGER(UII_CLICK);
    EXPOSE_INTEGER(UII_BACK);

    EXPOSE_FUNCTION(create_ui);
    EXPOSE_FUNCTION(destroy_ui);
    EXPOSE_FUNCTION(ui_table);
    EXPOSE_FUNCTION(get_ui_cursor);
    EXPOSE_FUNCTION(get_ui_button);

    mod_init_script();

    INFO("Opened");
}

void script_teardown() {
    CLOSE_HANDLE(context, lua_close);

    INFO("Closed");
}

void set_import_path(const char* path) {
    lua_getglobal(context, "package");
    lua_getfield(context, -1, "path");
    lua_pop(context, 1);
    lua_pushstring(context, path);
    lua_setfield(context, -2, "path");
    lua_pop(context, 1);
}

void _execute_buffer(void* buffer, size_t size, const char* name, const char* filename, int line) {
    if (luaL_loadbuffer(context, buffer, size, name))
        log_fatal(src_basename(filename), line, "Failed to load script \"%s\": %s", name, lua_tostring(context, -1));
    if (lua_pcall(context, 0, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error in script \"%s\": %s", name, lua_tostring(context, -1));
}

int create_table_ref() {
    lua_newtable(context);
    return luaL_ref(context, LUA_REGISTRYINDEX);
}

int function_ref(int idx, const char* name) {
    lua_getfield(context, idx, name);
    if (lua_isfunction(context, idx))
        return luaL_ref(context, LUA_REGISTRYINDEX);
    lua_pop(context, 1);
    return LUA_NOREF;
}

void unreference(int* ref) {
    luaL_unref(context, LUA_REGISTRYINDEX, *ref);
    *ref = LUA_NOREF;
}

void _execute_ref(int ref, const char* name, const char* filename, int line) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(context, 0, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}

void _execute_ref_in(int ref, HandleID scope, const char* name, const char* filename, int line) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, ref);
    lua_pushinteger(context, scope);
    if (lua_pcall(context, 1, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}
