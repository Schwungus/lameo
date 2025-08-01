#include "L_script.h"
#include "L_actor.h"
#include "L_asset.h"
#include "L_audio.h"
#include "L_flags.h"
#include "L_handler.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_memory.h"
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
    log_script(src_basename(debug.source), debug.name, debug.currentline, "%s", luaL_tolstring(L, -1, NULL));
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
SCRIPT_ASSET(animations, animation, struct Animation*, AnimationID);
SCRIPT_ASSET(fonts, font, struct Font*, FontID);
SCRIPT_ASSET(sounds, sound, struct Sound*, SoundID);
SCRIPT_ASSET(music, track, struct Track*, TrackID);

// Video
SCRIPT_GETTER(get_draw_time, integer);

SCRIPT_FUNCTION(set_main_color) {
    const GLfloat r = (GLfloat)luaL_checknumber(L, 1);
    const GLfloat g = (GLfloat)luaL_checknumber(L, 2);
    const GLfloat b = (GLfloat)luaL_checknumber(L, 3);

    set_main_color(r, g, b);

    return 0;
}

SCRIPT_FUNCTION(set_main_alpha) {
    const GLfloat a = (GLfloat)luaL_checknumber(L, 1);
    set_main_alpha(a);
    return 0;
}

SCRIPT_FUNCTION(main_rectangle) {
    const GLfloat x1 = (GLfloat)luaL_checknumber(L, 1);
    const GLfloat y1 = (GLfloat)luaL_checknumber(L, 2);
    const GLfloat x2 = (GLfloat)luaL_checknumber(L, 3);
    const GLfloat y2 = (GLfloat)luaL_checknumber(L, 4);
    const GLfloat z = (GLfloat)luaL_optnumber(L, 5, 0);
    const GLubyte r = (GLubyte)luaL_optinteger(L, 6, 255);
    const GLubyte g = (GLubyte)luaL_optinteger(L, 7, 255);
    const GLubyte b = (GLubyte)luaL_optinteger(L, 8, 255);
    const GLubyte a = (GLubyte)luaL_optinteger(L, 9, 255);

    main_rectangle(x1, y1, x2, y2, z, r, g, b, a);
    return 0;
}

SCRIPT_FUNCTION(main_sprite) {
    struct Texture* texture = s_test_texture(L, 1);
    const GLfloat x = (GLfloat)luaL_checknumber(L, 2);
    const GLfloat y = (GLfloat)luaL_checknumber(L, 3);
    const GLfloat z = (GLfloat)luaL_optnumber(L, 4, 0);

    main_sprite(texture, x, y, z);
    return 0;
}

SCRIPT_FUNCTION(main_string) {
    const char* str = luaL_checkstring(L, 1);
    struct Font* font = luaL_opt(L, s_check_font, 2, NULL);
    const GLfloat size = (GLfloat)luaL_checknumber(L, 3);
    const GLfloat x = (GLfloat)luaL_checknumber(L, 4);
    const GLfloat y = (GLfloat)luaL_checknumber(L, 5);
    const GLfloat z = (GLfloat)luaL_checknumber(L, 6);

    main_string(str, font, size, x, y, z);

    return 0;
}

SCRIPT_FUNCTION(string_width) {
    const char* str = luaL_checkstring(L, 1);
    struct Font* font = luaL_opt(L, s_check_font, 2, NULL);
    const GLfloat size = (GLfloat)luaL_checknumber(L, 3);

    lua_pushnumber(L, string_width(str, font, size));

    return 1;
}

SCRIPT_FUNCTION(string_height) {
    const char* str = luaL_checkstring(L, 1);
    const GLfloat size = (GLfloat)luaL_checknumber(L, 2);

    lua_pushnumber(L, string_height(str, size));

    return 1;
}

SCRIPT_CHECKER_DIRECT(surface, struct Surface*);
SCRIPT_TESTER_DIRECT(surface, struct Surface*);

SCRIPT_FUNCTION(create_surface) {
    const uint16_t width = luaL_checkinteger(L, 1);
    const uint16_t height = luaL_checkinteger(L, 2);
    const bool color = luaL_optinteger(L, 3, 1);
    const bool depth = luaL_optinteger(L, 4, 0);

    create_surface(true, width, height, color, depth);
    return 1;
}

SCRIPT_FUNCTION(validate_surface) {
    struct Surface* surface = s_check_surface(L, 1);
    validate_surface(surface);
    return 0;
}

SCRIPT_FUNCTION(dispose_surface) {
    struct Surface* surface = s_check_surface(L, 1);
    dispose_surface(surface);
    return 0;
}

SCRIPT_FUNCTION(push_surface) {
    struct Surface* surface = s_check_surface(L, 1);
    set_surface(surface);
    return 0;
}

SCRIPT_FUNCTION_DIRECT(pop_surface);

SCRIPT_FUNCTION(main_surface) {
    struct Surface* surface = s_check_surface(L, 1);
    const GLfloat x = (GLfloat)luaL_checknumber(L, 2);
    const GLfloat y = (GLfloat)luaL_checknumber(L, 3);
    const GLfloat z = (GLfloat)luaL_checknumber(L, 4);

    main_surface(surface, x, y, z);

    return 0;
}

SCRIPT_FUNCTION(clear_color) {
    const GLfloat r = (GLfloat)luaL_optnumber(L, 1, 0);
    const GLfloat g = (GLfloat)luaL_optnumber(L, 2, 0);
    const GLfloat b = (GLfloat)luaL_optnumber(L, 3, 0);
    const GLfloat a = (GLfloat)luaL_optnumber(L, 4, 1);

    clear_color(r, g, b, a);

    return 0;
}

SCRIPT_FUNCTION(clear_depth) {
    const GLfloat depth = (GLfloat)luaL_optnumber(L, 1, 1);
    clear_depth(depth);
    return 0;
}

SCRIPT_FUNCTION(clear_stencil) {
    const GLint stencil = (GLint)luaL_optinteger(L, 1, 0);
    clear_stencil(stencil);
    return 0;
}

// Model Instance
SCRIPT_CHECKER(model_instance, struct ModelInstance*);
SCRIPT_TESTER(model_instance, struct ModelInstance*);

SCRIPT_FUNCTION(model_instance_set_hidden) {
    struct ModelInstance* inst = s_check_model_instance(L, 1);
    const lua_Integer index = luaL_checkinteger(L, 2);
    const bool hidden = luaL_checkinteger(L, 3);

    if (index < 0 || index >= inst->model->num_submodels)
        luaL_argerror(L, 3, "invalid submodel index");
    inst->hidden[index] = hidden;

    return 0;
}

SCRIPT_FUNCTION(model_instance_set_animation) {
    struct ModelInstance* inst = s_check_model_instance(L, 1);
    struct Animation* animation = luaL_opt(L, s_check_animation, 2, NULL);
    const float frame = (float)luaL_optnumber(L, 3, 0);
    const bool loop = luaL_optinteger(L, 4, 0);

    set_model_instance_animation(inst, animation, frame, loop);
    return 0;
}

SCRIPT_FUNCTION(model_instance_override_texture) {
    struct ModelInstance* inst = s_check_model_instance(L, 1);
    const lua_Integer material_index = luaL_checkinteger(L, 2);
    if (material_index < 0 || material_index >= inst->model->num_materials)
        luaL_argerror(L, 2, "invalid material index");
    struct Texture* texture = s_test_texture(L, 3);

    inst->override_textures[material_index] = (texture == NULL) ? 0 : texture->texture;
    return 0;
}

SCRIPT_FUNCTION(model_instance_override_texture_surface) {
    struct ModelInstance* inst = s_check_model_instance(L, 1);
    const lua_Integer material_index = luaL_checkinteger(L, 2);
    if (material_index < 0 || material_index >= inst->model->num_materials)
        luaL_argerror(L, 2, "invalid material index");
    struct Surface* surface = s_test_surface(L, 3);

    validate_surface(surface);
    inst->override_textures[material_index] = (surface == NULL) ? 0 : surface->texture[SURFACE_COLOR_TEXTURE];
    return 0;
}

// Audio
SCRIPT_FUNCTION(play_ui_sound) {
    struct Sound* sound = luaL_opt(L, s_check_sound, 1, NULL);
    const bool loop = (bool)luaL_optnumber(L, 2, false);
    const uint32_t offset = luaL_optinteger(L, 3, 0);
    const float pitch = (float)luaL_optnumber(L, 4, 1);
    const float gain = (float)luaL_optnumber(L, 5, 1);

    lua_pushlightuserdata(L, play_ui_sound(sound, loop, offset, pitch, gain));
    return 1;
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

SCRIPT_FUNCTION(actor_get_player) {
    struct Actor* actor = s_check_actor(L, 1);
    if (actor->player == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, actor->player->userdata);
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

SCRIPT_FUNCTION(actor_get_model) {
    struct Actor* actor = s_check_actor(L, 1);
    if (actor->model == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, actor->model->userdata);
    return 1;
}

SCRIPT_FUNCTION(actor_create_model) {
    struct Actor* actor = s_check_actor(L, 1);
    struct Model* model = s_check_model(L, 2);
    create_actor_model(actor, model);
    return s_actor_get_model(L);
}

SCRIPT_FUNCTION(actor_destroy_model) {
    struct Actor* actor = s_check_actor(L, 1);
    destroy_actor_model(actor);
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

SCRIPT_FUNCTION(actor_set_pos) {
    struct Actor* actor = s_check_actor(L, 1);
    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_optnumber(L, 3, actor->pos[1]);
    const float z = (float)luaL_optnumber(L, 4, actor->pos[2]);

    set_actor_pos(actor, x, y, z);

    return 0;
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

SCRIPT_FUNCTION(actor_set_vel) {
    struct Actor* actor = s_check_actor(L, 1);
    actor->vel[0] = (float)luaL_checknumber(L, 2);
    actor->vel[1] = (float)luaL_optnumber(L, 3, actor->vel[1]);
    actor->vel[2] = (float)luaL_optnumber(L, 4, actor->vel[2]);
    return 0;
}

SCRIPT_FUNCTION(actor_get_angle) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->angle[0]);
    lua_pushnumber(L, actor->angle[1]);
    lua_pushnumber(L, actor->angle[2]);
    return 3;
}

SCRIPT_FUNCTION(actor_get_angle_x) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->angle[0]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_angle_y) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->angle[1]);
    return 1;
}

SCRIPT_FUNCTION(actor_get_angle_z) {
    struct Actor* actor = s_check_actor(L, 1);
    lua_pushnumber(L, actor->angle[2]);
    return 1;
}

SCRIPT_FUNCTION(actor_set_angle) {
    struct Actor* actor = s_check_actor(L, 1);
    actor->angle[0] = (float)luaL_checknumber(L, 2);
    actor->angle[1] = (float)luaL_optnumber(L, 3, actor->angle[1]);
    actor->angle[2] = (float)luaL_optnumber(L, 4, actor->angle[2]);
    return 0;
}

SCRIPT_FUNCTION(actor_to_sky) {
    struct Actor* actor = s_check_actor(L, 1);
    actor_to_sky(actor);
    return 0;
}

SCRIPT_FUNCTION(play_actor_sound) {
    struct Actor* actor = s_check_actor(L, 1);
    struct Sound* sound = luaL_opt(L, s_check_sound, 2, NULL);
    const float min_distance = (float)luaL_optnumber(L, 3, 1);
    const float max_distance = (float)luaL_optnumber(L, 4, 1);
    const bool loop = (bool)luaL_optnumber(L, 5, false);
    const uint32_t offset = luaL_optinteger(L, 6, 0);
    const float pitch = (float)luaL_optnumber(L, 7, 1);
    const float gain = (float)luaL_optnumber(L, 8, 1);

    lua_pushlightuserdata(L, play_actor_sound(actor, sound, min_distance, max_distance, loop, offset, pitch, gain));
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

SCRIPT_FUNCTION(camera_get_surface) {
    struct ActorCamera* camera = s_check_camera(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, camera->surface_ref);
    return 1;
}

SCRIPT_FUNCTION(get_active_camera) {
    struct ActorCamera* camera = get_active_camera();
    if (camera == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, camera->userdata);
    return 1;
}

SCRIPT_FUNCTION(set_active_camera) {
    struct ActorCamera* camera = luaL_opt(L, s_check_camera, 1, NULL);
    set_active_camera(camera);
    return 0;
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
    int buttons = (int)luaL_checkinteger(L, 1);
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
    int buttons = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, get_last_ui_buttons(buttons));
    return 1;
}

// Player
SCRIPT_CHECKER(player, struct Player*);
SCRIPT_TESTER(player, struct Player*);

SCRIPT_FUNCTION(get_player) {
    const int slot = (int)luaL_checkinteger(L, 1);

    struct Player* player = get_player(slot);
    if (player == NULL)
        lua_pushnil(L);
    else
        lua_rawgeti(L, LUA_REGISTRYINDEX, player->userdata);

    return 1;
}

SCRIPT_FUNCTION(get_player_slot) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushinteger(L, player->slot);
    return 1;
}

SCRIPT_FUNCTION(get_player_status) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushinteger(L, player->status);
    return 1;
}

SCRIPT_FUNCTION(get_player_move) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushnumber(L, (float)(player->input.move[0]) * PLAYER_MOVE_FACTOR);
    lua_pushnumber(L, (float)(player->input.move[1]) * PLAYER_MOVE_FACTOR);
    return 2;
}

SCRIPT_FUNCTION(get_player_aim) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushnumber(L, (float)(player->input.aim[0]) * PLAYER_AIM_FACTOR);
    lua_pushnumber(L, (float)(player->input.aim[1]) * PLAYER_AIM_FACTOR);
    return 2;
}

SCRIPT_FUNCTION(get_player_buttons) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushinteger(L, player->input.buttons);
    return 1;
}

SCRIPT_FUNCTION(get_player_last_move) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushnumber(L, (float)(player->last_input.move[0]) * PLAYER_MOVE_FACTOR);
    lua_pushnumber(L, (float)(player->last_input.move[1]) * PLAYER_MOVE_FACTOR);
    return 2;
}

SCRIPT_FUNCTION(get_player_last_aim) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushnumber(L, (float)(player->last_input.aim[0]) * PLAYER_AIM_FACTOR);
    lua_pushnumber(L, (float)(player->last_input.aim[1]) * PLAYER_AIM_FACTOR);
    return 2;
}

SCRIPT_FUNCTION(get_player_last_buttons) {
    const struct Player* player = s_check_player(L, 1);
    lua_pushinteger(L, player->last_input.buttons);
    return 1;
}

SCRIPT_FUNCTION(get_pflag_type) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    lua_pushinteger(L, get_pflag_type(player, name));
    return 1;
}

SCRIPT_FUNCTION(get_pflag_bool) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const bool failsafe = luaL_optinteger(L, 3, false);
    lua_pushboolean(L, get_pflag_bool(player, name, failsafe));
    return 1;
}

SCRIPT_FUNCTION(get_pflag_int) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const int failsafe = (int)luaL_optinteger(L, 3, 0);
    lua_pushinteger(L, get_pflag_int(player, name, failsafe));
    return 1;
}

SCRIPT_FUNCTION(get_pflag_float) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const float failsafe = (float)luaL_optnumber(L, 3, 0);
    lua_pushnumber(L, get_pflag_float(player, name, failsafe));
    return 1;
}

SCRIPT_FUNCTION(get_pflag_string) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const char* failsafe = luaL_optstring(L, 3, NULL);
    lua_pushstring(L, get_pflag_string(player, name, failsafe));
    return 1;
}

SCRIPT_FUNCTION(clear_pflags) {
    struct Player* player = s_check_player(L, 1);
    clear_pflags(player);
    return 0;
}

SCRIPT_FUNCTION(delete_pflag) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    delete_pflag(player, name);
    return 0;
}

SCRIPT_FUNCTION(set_pflag_bool) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const bool value = luaL_checkinteger(L, 3);
    set_pflag_bool(player, name, value);
    return 0;
}

SCRIPT_FUNCTION(set_pflag_int) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const int value = (int)luaL_checkinteger(L, 3);
    set_pflag_int(player, name, value);
    return 0;
}

SCRIPT_FUNCTION(set_pflag_float) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const float value = (float)luaL_checknumber(L, 3);
    set_pflag_float(player, name, value);
    return 0;
}

SCRIPT_FUNCTION(set_pflag_string) {
    struct Player* player = s_check_player(L, 1);
    const char* name = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    set_pflag_string(player, name, value);
    return 0;
}

// Flags
SCRIPT_FLAGS_READ(static);
SCRIPT_FLAGS_READ(global);
SCRIPT_FLAGS_WRITE(global);
SCRIPT_FLAGS_READ(local);
SCRIPT_FLAGS_WRITE(local);

// Math
SCRIPT_FUNCTION(lengthdir) {
    const float len = (float)luaL_checknumber(L, 1);
    const float dir = (float)luaL_checknumber(L, 2);
    const float rad = glm_rad(dir);

    lua_pushnumber(L, SDL_cosf(rad) * len);
    lua_pushnumber(L, SDL_sinf(rad) * len);

    return 2;
}

SCRIPT_FUNCTION(lengthdir_3d) {
    const float len = (float)luaL_checknumber(L, 1);
    const float yaw = (float)luaL_checknumber(L, 2);
    const float pitch = (float)luaL_checknumber(L, 3);

    const float yrad = glm_rad(yaw);
    const float prad = glm_rad(pitch);
    const float hor = SDL_cosf(prad);

    lua_pushnumber(L, (SDL_cosf(yrad) * len) * hor);
    lua_pushnumber(L, (SDL_sinf(yrad) * len) * hor);
    lua_pushnumber(L, -SDL_sinf(prad) * len);

    return 3;
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

    // Players
    luaL_newmetatable(context, "player");
    static const luaL_Reg player_methods[] = {
        {"get_slot", s_get_player_slot},
        {"get_status", s_get_player_status},

        {"get_move", s_get_player_move},
        {"get_aim", s_get_player_aim},
        {"get_buttons", s_get_player_buttons},
        {"get_last_move", s_get_player_last_move},
        {"get_last_aim", s_get_player_last_aim},
        {"get_last_buttons", s_get_player_last_buttons},

        {"get_type", s_get_pflag_type},
        {"get_bool", s_get_pflag_bool},
        {"get_int", s_get_pflag_int},
        {"get_float", s_get_pflag_float},
        {"get_string", s_get_pflag_string},

        {"clear", s_clear_pflags},
        {"delete", s_delete_pflag},
        {"set_bool", s_set_pflag_bool},
        {"set_int", s_set_pflag_int},
        {"set_float", s_set_pflag_float},
        {"set_string", s_set_pflag_string},

        {NULL, NULL},
    };
    luaL_setfuncs(context, player_methods, 0);
    lua_pushvalue(context, -1);
    lua_setfield(context, -2, "__index");
    lua_pop(context, 1);

    EXPOSE_FUNCTION(get_player);

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
    EXPOSE_FUNCTION(main_sprite);

    EXPOSE_FUNCTION(main_string);
    EXPOSE_FUNCTION(string_width);
    EXPOSE_FUNCTION(string_height);

    luaL_newmetatable(context, "surface");
    static const luaL_Reg surface_methods[] = {
        {"validate", s_validate_surface},
        {"dispose", s_dispose_surface},
        {"push", s_push_surface},
        {"main", s_main_surface},
        {"__gc", s_dispose_surface},

        {NULL, NULL},
    };
    luaL_setfuncs(context, surface_methods, 0);
    lua_pushvalue(context, -1);
    lua_setfield(context, -2, "__index");
    lua_pop(context, 1);

    EXPOSE_FUNCTION(create_surface);
    EXPOSE_FUNCTION(pop_surface);

    EXPOSE_FUNCTION(clear_color);
    EXPOSE_FUNCTION(clear_depth);
    EXPOSE_FUNCTION(clear_stencil);

    luaL_newmetatable(context, "model_instance");
    static const luaL_Reg model_instance_methods[] = {
        {"set_hidden", s_model_instance_set_hidden},
        {"set_animation", s_model_instance_set_animation},

        {"override_texture", s_model_instance_override_texture},
        {"override_texture_surface", s_model_instance_override_texture_surface},

        {NULL, NULL},
    };
    luaL_setfuncs(context, model_instance_methods, 0);
    lua_pushvalue(context, -1);
    lua_setfield(context, -2, "__index");
    lua_pop(context, 1);

    // Audio
    EXPOSE_FUNCTION(play_ui_sound);

    // Actor
    luaL_newmetatable(context, "actor");
    static const luaL_Reg actor_methods[] = {
        {"is_ancestor", s_actor_is_ancestor},

        {"get_player", s_actor_get_player},

        {"get_camera", s_actor_get_camera},
        {"create_camera", s_actor_create_camera},
        {"destroy_camera", s_actor_destroy_camera},

        {"get_model", s_actor_get_model},
        {"create_model", s_actor_create_model},
        {"destroy_model", s_actor_destroy_model},
        {"to_sky", s_actor_to_sky},

        {"get_pos", s_actor_get_pos},
        {"get_x", s_actor_get_pos_x},
        {"get_y", s_actor_get_pos_y},
        {"get_z", s_actor_get_pos_z},
        {"set_pos", s_actor_set_pos},

        {"get_vel", s_actor_get_vel},
        {"get_x_vel", s_actor_get_vel_x},
        {"get_y_vel", s_actor_get_vel_y},
        {"get_z_vel", s_actor_get_vel_z},
        {"set_vel", s_actor_set_vel},

        {"get_angle", s_actor_get_angle},
        {"get_yaw", s_actor_get_angle_x},
        {"get_pitch", s_actor_get_angle_y},
        {"get_roll", s_actor_get_angle_z},
        {"set_angle", s_actor_set_angle},

        {"play_sound", s_play_actor_sound},

        {NULL, NULL},
    };
    luaL_setfuncs(context, actor_methods, 0);
    lua_pushcfunction(context, s_actor_index);
    lua_setfield(context, -2, "__index");
    lua_pushcfunction(context, s_actor_newindex);
    lua_setfield(context, -2, "__newindex");
    lua_pop(context, 1);

    EXPOSE_FUNCTION(actor_exists);

    luaL_newmetatable(context, "camera");
    static const luaL_Reg camera_methods[] = {
        {"get_actor", s_camera_get_actor},
        {"get_surface", s_camera_get_surface},

        {"set_active", s_set_active_camera},

        {NULL, NULL},
    };
    luaL_setfuncs(context, camera_methods, 0);
    lua_pushcfunction(context, s_camera_index);
    lua_setfield(context, -2, "__index");
    lua_pushcfunction(context, s_camera_newindex);
    lua_setfield(context, -2, "__newindex");
    lua_pop(context, 1);

    EXPOSE_FUNCTION(get_active_camera);
    EXPOSE_FUNCTION(set_active_camera);

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
    EXPOSE_ASSET(animations, animation);
    EXPOSE_ASSET(fonts, font);
    EXPOSE_ASSET(sounds, sound);
    EXPOSE_ASSET(music, track);

    // Flags
    EXPOSE_INTEGER(FT_NULL);
    EXPOSE_INTEGER(FT_BOOL);
    EXPOSE_INTEGER(FT_INT);
    EXPOSE_INTEGER(FT_FLOAT);
    EXPOSE_INTEGER(FT_STRING);

    EXPOSE_FLAGS_READ(static);
    EXPOSE_FLAGS_READ(global);
    EXPOSE_FLAGS_WRITE(global);
    EXPOSE_FLAGS_READ(local);
    EXPOSE_FLAGS_WRITE(local);

    // Math
    EXPOSE_FUNCTION(lengthdir);
    EXPOSE_FUNCTION(lengthdir_3d);

    INFO("Opened");
}

void script_teardown() {
    // May segfault, but it doesn't matter as the game is already over halfway
    // closed at this point.
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

void collect_garbage() {
    lua_gc(context, LUA_GCCOLLECT);
}

void* userdata_alloc(const char* type, size_t size) {
    void* userdata = lua_newuserdata(context, size);
    luaL_setmetatable(context, type);
    return userdata;
}

void* _userdata_alloc_clean(const char* type, size_t size, const char* filename, int line) {
    void* userdata = lua_newuserdata(context, size);
    _lame_set(userdata, 0, size, filename, line);
    luaL_setmetatable(context, type);
    return userdata;
}

int create_ref() {
    return luaL_ref(context, LUA_REGISTRYINDEX);
}

int create_table_ref() {
    lua_newtable(context);
    return create_ref();
}

int create_pointer_ref(const char* type, void* ptr) {
    *((void**)(userdata_alloc(type, sizeof(void*)))) = ptr;
    return create_ref();
}

int function_ref(int idx, const char* name) {
    lua_getfield(context, idx, name);
    if (lua_isfunction(context, -1))
        return create_ref();
    lua_pop(context, 1);
    return LUA_NOREF;
}

int json_to_table_ref(yyjson_val* obj) {
    if (!yyjson_is_obj(obj) || yyjson_obj_size(obj) <= 0)
        return LUA_NOREF;
    lua_newtable(context);

    size_t i, n;
    yyjson_val *key, *value;
    yyjson_obj_foreach(obj, i, n, key, value) {
        switch (yyjson_get_type(value)) {
            default:
                WTF("Unsupported JSON type \"%s\"", yyjson_get_type_desc(value));
            case YYJSON_TYPE_NULL:
                lua_pushnil(context);
                break;
            case YYJSON_TYPE_BOOL:
                lua_pushboolean(context, yyjson_get_bool(value));
                break;
            case YYJSON_TYPE_NUM:
                if (yyjson_is_int(value))
                    lua_pushinteger(context, yyjson_get_int(value));
                else
                    lua_pushnumber(context, yyjson_get_num(value));
                break;
            case YYJSON_TYPE_STR:
                lua_pushstring(context, yyjson_get_str(value));
                break;
        }
        lua_setfield(context, -2, yyjson_get_str(key));
    }

    return create_ref();
}

void copy_table(int sref, int dref) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, sref);
    int src = lua_gettop(context);
    lua_rawgeti(context, LUA_REGISTRYINDEX, dref);
    int dest = lua_gettop(context);

    lua_pushnil(context);
    while (lua_next(context, src) != 0)
        lua_setfield(context, dest, lua_tostring(context, -2));

    lua_pop(context, 2);
}

void unreference(int* ref) {
    luaL_unref(context, LUA_REGISTRYINDEX, *ref);
    *ref = LUA_NOREF;
}

void unreference_pointer(int* ref) {
    lua_rawgeti(context, LUA_REGISTRYINDEX, *ref);
    *((void**)(lua_touserdata(context, -1))) = NULL;
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
    if (lua_pcall(context, 2, 0, 0) != LUA_OK)
        log_fatal(src_basename(filename), line, "Error from \"%s\": %s", name, lua_tostring(context, -1));
}

lua_Debug* script_stack_trace(lua_State* L) {
    if (L == NULL)
        L = context;
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nSl", &debug);
    return &debug;
}
