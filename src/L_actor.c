#include "L_actor.h"
#include "L_log.h"
#include "L_memory.h"

static struct HashMap* actor_types = NULL;
static struct Fixture* actor_handles = NULL;

void actor_init() {
    actor_types = create_hash_map();
    actor_handles = create_fixture();

    INFO("Opened");
}

void actor_teardown() {
    for (size_t i = 0; actor_types->count > 0 && i < actor_types->capacity; i++) {
        struct KeyValuePair* kvp = &actor_types->items[i];
        if (kvp->key == NULL)
            continue;

        struct ActorType* type = kvp->value;
        if (type != NULL) {
            lame_free(&type->name);
            unreference(&type->load);
            unreference(&type->create);
            unreference(&type->on_destroy);
            unreference(&type->cleanup);
            unreference(&type->tick);
            unreference(&type->draw);
            unreference(&type->draw_screen);
            unreference(&type->draw_ui);
            lame_free(&kvp->value);
        }

        lame_free(&kvp->key);
        actor_types->count--;
    }

    destroy_hash_map(actor_types, false);
    CLOSE_POINTER(actor_handles, destroy_fixture);

    INFO("Closed");
}

int define_actor(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const char* parent_name = luaL_optstring(L, 2, NULL);
    luaL_checktype(L, 3, LUA_TTABLE);

    struct ActorType* parent = NULL;
    if (parent_name != NULL) {
        if (SDL_strcmp(name, parent_name) == 0)
            FATAL("Actor \"%s\" inheriting itself?", name);
        parent = from_hash_map(actor_types, parent_name);
        if (parent == NULL)
            FATAL("Actor \"%s\" inheriting from unknown type \"%s\"", name, parent_name);
    }

    struct ActorType* type = from_hash_map(actor_types, name);
    if (type == NULL) {
        type = lame_alloc(sizeof(struct ActorType));
        type->name = SDL_strdup(name);
        to_hash_map(actor_types, name, type, true);
    } else {
        WARN("Redefining Actor \"%s\"", name);
        if (type->parent == NULL || type->load != parent->load)
            unreference(&type->load);
        if (type->parent == NULL || type->create != parent->create)
            unreference(&type->create);
        if (type->parent == NULL || type->on_destroy != parent->on_destroy)
            unreference(&type->on_destroy);
        if (type->parent == NULL || type->cleanup != parent->cleanup)
            unreference(&type->cleanup);
        if (type->parent == NULL || type->tick != parent->tick)
            unreference(&type->tick);
        if (type->parent == NULL || type->draw != parent->draw)
            unreference(&type->draw);
        if (type->parent == NULL || type->draw_screen != parent->draw_screen)
            unreference(&type->draw_screen);
        if (type->parent == NULL || type->draw_ui != parent->draw_ui)
            unreference(&type->draw_ui);
    }

    type->parent = parent;

    type->load = function_ref(-1, "load");
    type->create = function_ref(-1, "create");
    type->on_destroy = function_ref(-1, "on_destroy");
    type->cleanup = function_ref(-1, "cleanup");
    type->tick = function_ref(-1, "tick");
    type->draw = function_ref(-1, "draw");
    type->draw_screen = function_ref(-1, "draw_screen");
    type->draw_ui = function_ref(-1, "draw_ui");

    if (parent != NULL) {
        if (type->load == LUA_NOREF)
            type->load = parent->load;
        if (type->create == LUA_NOREF)
            type->create = parent->create;
        if (type->on_destroy == LUA_NOREF)
            type->on_destroy = parent->on_destroy;
        if (type->cleanup == LUA_NOREF)
            type->cleanup = parent->cleanup;
        if (type->tick == LUA_NOREF)
            type->tick = parent->tick;
        if (type->draw == LUA_NOREF)
            type->draw = parent->draw;
        if (type->draw_screen == LUA_NOREF)
            type->draw_screen = parent->draw_screen;
        if (type->draw_ui == LUA_NOREF)
            type->draw_ui = parent->draw_ui;
    }

    SCRIPT_LOG(L, "Defined Actor \"%s\"", name);
    return 0;
}

bool load_actor(const char* name) {
    struct ActorType* type = from_hash_map(actor_types, name);
    if (type == NULL) {
        WTF("Unknown Actor type \"%s\"", name);
        return false;
    }

    struct ActorType* it = type;
    do {
        if (it->load != LUA_NOREF)
            execute_ref(it->load, it->name);
        INFO("Loaded Actor \"%s\"", it->name);
        it = it->parent;
    } while (it != NULL);

    return true;
}
