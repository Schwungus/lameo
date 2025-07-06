#include "L_actor.h"
#include "L_log.h"
#include "L_memory.h"

static struct HashMap* actor_types = NULL;
static struct Actor* actors = NULL;
static struct Fixture* actor_handles = NULL;

void actor_init() {
    actor_types = create_hash_map();
    actor_handles = create_fixture();

    INFO("Opened");
}

void actor_teardown() {
    while (actors != NULL)
        destroy_actor(actors, false);

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

struct ActorType* get_actor_type(const char* name) {
    return (struct ActorType*)from_hash_map(actor_types, name);
}

struct Actor* create_actor(
    struct Room* room, struct RoomActor* base, const char* name, bool invoke_create, float x, float y, float z,
    float yaw, float pitch, float roll, uint16_t tag
) {
    struct ActorType* type = get_actor_type(name);
    if (type == NULL) {
        WTF("Unknown Actor type \"%s\"", name);
        return NULL;
    }

    return create_actor_from_type(room, base, type, invoke_create, x, y, z, yaw, pitch, roll, tag);
}

struct Actor* create_actor_from_type(
    struct Room* room, struct RoomActor* base, struct ActorType* type, bool invoke_create, float x, float y, float z,
    float yaw, float pitch, float roll, uint16_t tag
) {
    if (base != NULL && ((base->flags & RAF_DISPOSED) || base->actor != NULL))
        return NULL;

    struct Actor* actor = lame_alloc(sizeof(struct Actor));
    ActorID hid = create_handle(actor_handles, actor);
    actor->hid = hid;
    actor->type = type;
    actor->camera = NULL;

    if (actors != NULL)
        actors->next = actor;
    actor->previous = actors;
    actor->next = NULL;
    actors = actor;

    if (room->actors != NULL)
        room->actors->next_neighbor = actor;
    actor->previous_neighbor = room->actors;
    actor->next_neighbor = NULL;
    room->actors = actor;

    actor->room = room;
    actor->base = base;

    actor->player = NULL;
    actor->master = actor->target = 0;

    actor->pos[0] = x;
    actor->pos[1] = y;
    actor->pos[2] = z;
    actor->angle[0] = yaw;
    actor->angle[1] = pitch;
    actor->angle[2] = roll;
    glm_vec3_zero(actor->vel);
    actor->friction = actor->gravity = 0;

    actor->table = create_table_ref();
    actor->flags = AF_DEFAULT;

    if (invoke_create) {
        if (type->create != LUA_NOREF)
            execute_ref_in(type->create, actor->hid, type->name);
    } else {
        // Assume that this actor's create() will be invoked later on
        actor->flags |= AF_NEW;
    }

    return hid_to_actor(hid) != NULL ? actor : NULL;
}

void tick_actor(struct Actor* actor) {
    if (actor->type->tick != LUA_NOREF)
        execute_ref_in(actor->type->tick, actor->hid, actor->type->name);
}

void destroy_actor(struct Actor* actor, bool natural) {
    if (natural && actor->type->on_destroy != LUA_NOREF)
        execute_ref_in(actor->type->on_destroy, actor->hid, actor->type->name);
    if (actor->type->cleanup != LUA_NOREF)
        execute_ref_in(actor->type->cleanup, actor->hid, actor->type->name);

    if (actor->base != NULL) {
        if ((actor->flags & AF_DISPOSABLE) || (actor->base->flags & RAF_DISPOSABLE))
            actor->base->flags |= RAF_DISPOSED;
        actor->base->actor = NULL;
    }

    if (actors == actor)
        actors = actor->previous;
    if (actor->previous != NULL)
        actor->previous->next = actor->next;
    if (actor->next != NULL)
        actor->next->previous = actor->previous;

    if (actor->room->actors == actor)
        actor->room->actors = actor->previous_neighbor;
    if (actor->previous_neighbor != NULL)
        actor->previous_neighbor->next_neighbor = actor->next_neighbor;
    if (actor->next_neighbor != NULL)
        actor->next_neighbor->previous_neighbor = actor->previous_neighbor;

    if (actor->camera != NULL) {
        // TODO: destroy_actor_camera();
    }

    if (actor->player != NULL && actor->player->actor == actor)
        actor->player->actor = NULL;

    unreference(&actor->table);

    destroy_handle(actor_handles, actor->hid);
    lame_free(&actor);
}

struct Actor* hid_to_actor(ActorID hid) {
    return (struct Actor*)hid_to_pointer(actor_handles, hid);
}

bool actor_is_ancestor(struct Actor* actor, const char* name) {
    struct ActorType* it = actor->type;
    while (it != NULL) {
        if (SDL_strcmp(it->name, name) == 0)
            return true;
        it = it->parent;
    }

    return false;
}
