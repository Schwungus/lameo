#include "L_script.h"
#include "L_actor.h"
#include "L_asset.h"
#include "L_audio.h"
#include "L_handler.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_mod.h"
#include "L_player.h"
#include "L_room.h"
#include "L_ui.h"
#include "L_video.h"

static lua_State* context = NULL;
static lua_Debug debug = {0};

// Script functions
// Types
SCRIPT_FUNCTION(tostring) {
    lua_pushstring(L, luaL_tolstring(L, -1, NULL));
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
    return define_actor(L);
}

// Debug
SCRIPT_FUNCTION(print) {
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nSl", &debug);
    log_script(debug.source, debug.name, debug.currentline, "%s", luaL_tolstring(L, -1, NULL));
    return 0;
}

SCRIPT_FUNCTION(error) {
    return luaL_error(L, luaL_tolstring(L, -1, NULL));
}

// Localization
SCRIPT_FUNCTION(localized) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushstring(L, localized(key));
    return 1;
}

// Assets
SCRIPT_ASSET(shaders, shader, struct Shader*, ShaderID);
SCRIPT_ASSET(textures, texture, struct Texture*, TextureID);
SCRIPT_ASSET(materials, material, struct Material*, MaterialID);
SCRIPT_ASSET(models, model, struct Model*, ModelID);
SCRIPT_ASSET(fonts, font, struct Font*, FontID);
SCRIPT_ASSET(sounds, sound, struct Sound*, SoundID);
SCRIPT_ASSET(music, track, struct Track*, TrackID);

// Video
SCRIPT_GETTER(get_draw_time, integer);

SCRIPT_FUNCTION(set_main_color) {
    const GLfloat r = luaL_checknumber(L, 1);
    const GLfloat g = luaL_checknumber(L, 2);
    const GLfloat b = luaL_checknumber(L, 3);

    set_main_color(r, g, b);

    return 0;
}

SCRIPT_FUNCTION(set_main_alpha) {
    const GLfloat a = luaL_checknumber(L, 1);
    set_main_alpha(a);
    return 0;
}

SCRIPT_FUNCTION(main_rectangle) {
    set_main_texture(NULL);

    const GLfloat x1 = luaL_checknumber(L, 1);
    const GLfloat y1 = luaL_checknumber(L, 2);
    const GLfloat x2 = luaL_checknumber(L, 3);
    const GLfloat y2 = luaL_checknumber(L, 4);
    const GLfloat z = luaL_checknumber(L, 5);
    const GLfloat r = luaL_optinteger(L, 6, 255);
    const GLfloat g = luaL_optinteger(L, 7, 255);
    const GLfloat b = luaL_optinteger(L, 8, 255);
    const GLfloat a = luaL_optinteger(L, 9, 255);

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
    struct Font* font = luaL_opt(L, s_check_font, 2, NULL);
    const GLfloat size = luaL_checknumber(L, 3);
    const GLfloat x = luaL_checknumber(L, 4);
    const GLfloat y = luaL_checknumber(L, 5);
    const GLfloat z = luaL_checknumber(L, 6);

    main_string(str, NULL, size, x, y, z);

    return 0;
}

SCRIPT_FUNCTION(string_width) {
    const char* str = luaL_checkstring(L, 1);
    struct Font* font = luaL_opt(L, s_check_font, 2, NULL);
    const GLfloat size = luaL_checknumber(L, 3);
    lua_pushnumber(L, string_width(str, font, size));
    return 1;
}

SCRIPT_FUNCTION(string_height) {
    const char* str = luaL_checkstring(L, 1);
    const GLfloat size = luaL_checknumber(L, 2);
    lua_pushnumber(L, string_height(str, size));
    return 1;
}

// Audio
SCRIPT_FUNCTION(play_ui_sound) {
    struct Sound* sound = luaL_opt(L, s_check_sound, 1, NULL);
    const bool loop = luaL_optnumber(L, 2, false);
    const uint32_t offset = luaL_optinteger(L, 3, 0);
    const float pitch = luaL_optnumber(L, 4, 1);
    const float gain = luaL_optnumber(L, 5, 1);

    play_ui_sound(sound, loop, offset, pitch, gain);

    return 0;
}

// Actor
SCRIPT_CHECKER(actor, struct Actor*);
SCRIPT_TESTER(actor, struct Actor*);
SCRIPT_CHECKER(camera, struct ActorCamera*);
SCRIPT_TESTER(camera, struct ActorCamera*);

SCRIPT_FUNCTION(actor_index) {
    struct Actor* actor = s_check_actor(L, 1);
    const char* key = luaL_checkstring(L, 2);

    lua_getmetatable(L, -2);
    lua_pushvalue(L, -2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1))
        return 1;
    lua_pop(L, 2);

    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->table);
    lua_getfield(L, -1, key);
    lua_remove(L, -2);

    return 1;
}

SCRIPT_FUNCTION(actor_newindex) {
    struct Actor* actor = s_check_actor(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    lua_rawgeti(L, LUA_REGISTRYINDEX, actor->table);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, key);
    lua_pop(L, -1);

    return 0;
}

SCRIPT_FUNCTION(actor_exists) {
    lua_pushboolean(L, s_test_actor(L, 1) != NULL);
    return 1;
}

SCRIPT_FUNCTION(actor_is_ancestor) {
    struct Actor* actor = s_check_actor(L, 1);
    const char* type = luaL_checkstring(L, 2);
    lua_pushboolean(L, actor_is_ancestor(actor, type));
    return 1;
}

SCRIPT_FUNCTION(actor_get_camera) {
    struct Actor* actor = s_check_actor(L, 1);
    if (actor->camera == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, actor->camera->userdata);
    return 1;
}

SCRIPT_FUNCTION(actor_create_camera) {
    struct Actor* actor = s_check_actor(L, 1);
    create_actor_camera(actor);
    return s_actor_get_camera(L);
}

SCRIPT_FUNCTION(actor_destroy_camera) {
    struct Actor* actor = s_check_actor(L, 1);
    destroy_actor_camera(actor);
    return 0;
}

SCRIPT_FUNCTION(actor_get_pos) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->pos[0]);
    lua_pushnumber(L, actor->pos[1]);
    lua_pushnumber(L, actor->pos[2]);
    return 3;
}

SCRIPT_FUNCTION(actor_get_pos_x) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->pos[0]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_pos_y) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->pos[1]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_pos_z) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->pos[2]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_vel) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->vel[0]);
    lua_pushnumber(L, actor->vel[1]);
    lua_pushnumber(L, actor->vel[2]);
    return 3;
}

SCRIPT_FUNCTION(actor_get_vel_x) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->vel[0]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_vel_y) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->vel[1]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_vel_z) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->vel[2]);
    return 1;
}

SCRIPT_FUNCTION(camera_index) {
    struct ActorCamera* camera = s_check_camera(L, 1);
    const char* key = luaL_checkstring(L, 2);

    lua_getmetatable(L, -2);
    lua_pushvalue(L, -2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1))
        return 1;
    lua_pop(L, 2);

    lua_rawgeti(L, LUA_REGISTRYINDEX, camera->table);
    lua_getfield(L, -1, key);
    lua_remove(L, -2);

    return 1;
}

SCRIPT_FUNCTION(camera_newindex) {
    struct ActorCamera* camera = s_check_camera(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    lua_rawgeti(L, LUA_REGISTRYINDEX, camera->table);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, key);
    lua_pop(L, -1);

    return 0;
}

SCRIPT_FUNCTION(camera_get_actor) {
    struct ActorCamera* camera = s_check_camera(L, 1);
    if (camera->actor == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, camera->actor->userdata);
    return 1;
}

// UI
SCRIPT_CHECKER(ui, struct UI*);
SCRIPT_TESTER(ui, struct UI*);

SCRIPT_FUNCTION(ui_index) {
    struct UI* ui = s_check_ui(L, 1);
    const char* key = luaL_checkstring(L, 2);

    lua_getmetatable(L, -2);
    lua_pushvalue(L, -2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1))
        return 1;
    lua_pop(L, 2);

    lua_rawgeti(L, LUA_REGISTRYINDEX, ui->table);
    lua_getfield(L, -1, key);
    lua_remove(L, -2);

    return 1;
}

SCRIPT_FUNCTION(ui_newindex) {
    struct UI* ui = s_check_ui(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    lua_rawgeti(L, LUA_REGISTRYINDEX, ui->table);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, key);
    lua_pop(L, -1);

    return 0;
}

SCRIPT_FUNCTION(ui_exists) {
    lua_pushboolean(L, s_test_ui(L, 1) != NULL);
    return 1;
}

SCRIPT_FUNCTION(ui_is_ancestor) {
    struct UI* ui = s_check_ui(L, 1);
    const char* type = luaL_checkstring(L, 2);
    lua_pushboolean(L, ui_is_ancestor(ui, type));
    return 1;
}

SCRIPT_FUNCTION(create_ui) {
    struct UI* parent = luaL_opt(L, s_check_ui, 1, NULL);
    const char* name = luaL_checkstring(L, 2);

    struct UI* ui = create_ui(parent, name);
    lua_pushinteger(L, ui == NULL ? 0 : ui->hid);

    return 1;
}

SCRIPT_FUNCTION(destroy_ui) {
    struct UI* ui = s_check_ui(L, 1);
    destroy_ui(ui);
    return 0;
}

SCRIPT_FUNCTION(get_ui_cursor) {
    const vec2* cursor = get_ui_cursor();
    lua_pushnumber(L, (*cursor)[0]);
    lua_pushnumber(L, (*cursor)[1]);
    return 2;
}

SCRIPT_FUNCTION(get_ui_buttons) {
    int buttons = luaL_checkinteger(L, 1);
    lua_pushboolean(L, get_ui_buttons(buttons));
    return 1;
}

SCRIPT_FUNCTION(get_last_ui_cursor) {
    const vec2* cursor = get_last_ui_cursor();
    lua_pushnumber(L, (*cursor)[0]);
    lua_pushnumber(L, (*cursor)[1]);
    return 2;
}

SCRIPT_FUNCTION(get_last_ui_buttons) {
    int buttons = luaL_checkinteger(L, 1);
    lua_pushboolean(L, get_last_ui_buttons(buttons));
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

    // Players
    luaL_newmetatable(context, "player");
    lua_pop(context, 1);

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

    // Localize
    EXPOSE_FUNCTION(localized);

    // Video
    EXPOSE_INTEGER(DEFAULT_DISPLAY_WIDTH);
    EXPOSE_INTEGER(DEFAULT_DISPLAY_HEIGHT);
    EXPOSE_NUMBER(UI_Z);

    EXPOSE_FUNCTION(get_draw_time);

    EXPOSE_FUNCTION(set_main_color);
    EXPOSE_FUNCTION(set_main_alpha);

    EXPOSE_FUNCTION(main_rectangle);

    EXPOSE_FUNCTION(main_string);
    EXPOSE_FUNCTION(string_width);
    EXPOSE_FUNCTION(string_height);

    // Audio
    EXPOSE_FUNCTION(play_ui_sound);

    // Actor
    luaL_newmetatable(context, "actor");
    static const luaL_Reg actor_methods[] = {
        {"is_ancestor", s_actor_is_ancestor},

        {"get_camera", s_actor_get_camera},
        {"create_camera", s_actor_create_camera},
        {"destroy_camera", s_actor_destroy_camera},

        {"get_pos", s_actor_get_pos},
        {"get_pos_x", s_actor_get_pos_x},
        {"get_pos_y", s_actor_get_pos_y},
        {"get_pos_z", s_actor_get_pos_z},

        {"get_vel", s_actor_get_vel},
        {"get_vel_x", s_actor_get_vel_x},
        {"get_vel_y", s_actor_get_vel_y},
        {"get_vel_z", s_actor_get_vel_z},

        {NULL, NULL},
    };
    luaL_setfuncs(context, actor_methods, 0);
    lua_pushcfunction(context, s_actor_index);
    lua_setfield(context, -2, "__index");
    lua_pushcfunction(context, s_actor_newindex);
    lua_setfield(context, -2, "__newindex");
    lua_pop(context, 1);

    luaL_newmetatable(context, "camera");
    static const luaL_Reg camera_methods[] = {
        {"get_actor", s_camera_get_actor},
        {NULL, NULL},
    };
    luaL_setfuncs(context, camera_methods, 0);
    lua_pushcfunction(context, s_camera_index);
    lua_setfield(context, -2, "__index");
    lua_pushcfunction(context, s_camera_newindex);
    lua_setfield(context, -2, "__newindex");
    lua_pop(context, 1);

    EXPOSE_FUNCTION(actor_exists);

    // UI
    luaL_newmetatable(context, "ui");
    static const luaL_Reg ui_methods[] = {
        {"is_ancestor", s_ui_is_ancestor},
        {NULL, NULL},
    };
    luaL_setfuncs(context, ui_methods, 0);
    lua_pushcfunction(context, s_ui_index);
    lua_setfield(context, -2, "__index");
    lua_pushcfunction(context, s_ui_newindex);
    lua_setfield(context, -2, "__newindex");
    lua_pop(context, 1);

    EXPOSE_INTEGER(UII_UP);
    EXPOSE_INTEGER(UII_LEFT);
    EXPOSE_INTEGER(UII_DOWN);
    EXPOSE_INTEGER(UII_RIGHT);
    EXPOSE_INTEGER(UII_ENTER);
    EXPOSE_INTEGER(UII_CLICK);
    EXPOSE_INTEGER(UII_BACK);

    EXPOSE_FUNCTION(create_ui);
    EXPOSE_FUNCTION(destroy_ui);

    EXPOSE_FUNCTION(get_ui_cursor);
    EXPOSE_FUNCTION(get_ui_buttons);
    EXPOSE_FUNCTION(get_last_ui_cursor);
    EXPOSE_FUNCTION(get_last_ui_buttons);

    // Assets
    EXPOSE_ASSET(shaders, shader);
    EXPOSE_ASSET(textures, texture);
    EXPOSE_ASSET(materials, material);
    EXPOSE_ASSET(models, model);
    EXPOSE_ASSET(fonts, font);
    EXPOSE_ASSET(sounds, sound);
    EXPOSE_ASSET(music, track);

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

int create_pointer_ref(const char* type, void* ptr) {
    *(void**)(lua_newuserdata(context, sizeof(void*))) = ptr;
    luaL_setmetatable(context, type);
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

void unreference_pointer(int* ref) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, *ref);
    *(void**)(lua_touserdata(context, -1)) = NULL;
    unreference(ref);
}

void _execute_ref(int ref, const char* name, const char* filename, int line) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(context, 0, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}

void _execute_ref_in(int ref, int userdata, const char* name, const char* filename, int line) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, ref);
    lua_rawgeti(context, LUA_REGISTRYINDEX, userdata);
    if (lua_pcall(context, 1, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}

void _execute_ref_in_child(int ref, int userdata, int userdata2, const char* name, const char* filename, int line) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, ref);
    lua_rawgeti(context, LUA_REGISTRYINDEX, userdata);
    lua_rawgeti(context, LUA_REGISTRYINDEX, userdata2);
    if (lua_pcall(context, 1, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}

lua_Debug* script_stack_trace(lua_State* L) {
    if (L == NULL)
        L = context;
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nSl", &debug);
    return &debug;
}
