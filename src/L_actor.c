#include "L_actor.h"
#include "L_log.h"

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
        destroy_actor(actors, false, false);

    for (size_t i = 0; actor_types->count > 0 && i < actor_types->capacity; i++) {
        struct KeyValuePair* kvp = &actor_types->items[i];
        if (kvp->key == NULL || kvp->key == HASH_TOMBSTONE)
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
        kvp->key = HASH_TOMBSTONE;
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
        if (type->parent == NULL || type->create_camera != parent->create_camera)
            unreference(&type->create_camera);
        if (type->parent == NULL || type->on_destroy != parent->on_destroy)
            unreference(&type->on_destroy);
        if (type->parent == NULL || type->cleanup != parent->cleanup)
            unreference(&type->cleanup);
        if (type->parent == NULL || type->tick != parent->tick)
            unreference(&type->tick);
        if (type->parent == NULL || type->tick_camera != parent->tick_camera)
            unreference(&type->tick_camera);
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
    type->create_camera = function_ref(-1, "create_camera");
    type->on_destroy = function_ref(-1, "on_destroy");
    type->cleanup = function_ref(-1, "cleanup");
    type->tick = function_ref(-1, "tick");
    type->tick_camera = function_ref(-1, "tick_camera");
    type->draw = function_ref(-1, "draw");
    type->draw_screen = function_ref(-1, "draw_screen");
    type->draw_ui = function_ref(-1, "draw_ui");

    if (parent != NULL) {
        if (type->load == LUA_NOREF)
            type->load = parent->load;
        if (type->create == LUA_NOREF)
            type->create = parent->create;
        if (type->create_camera == LUA_NOREF)
            type->create_camera = parent->create_camera;
        if (type->on_destroy == LUA_NOREF)
            type->on_destroy = parent->on_destroy;
        if (type->cleanup == LUA_NOREF)
            type->cleanup = parent->cleanup;
        if (type->tick == LUA_NOREF)
            type->tick = parent->tick;
        if (type->tick_camera == LUA_NOREF)
            type->tick_camera = parent->tick_camera;
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
    actor->model = NULL;

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

    actor->pos[0] = x;
    actor->pos[1] = y;
    actor->pos[2] = z;
    actor->angle[0] = yaw;
    actor->angle[1] = pitch;
    actor->angle[2] = roll;

    glm_vec3_zero(actor->vel);
    glm_vec3_one(actor->friction);
    glm_vec3_zero(actor->gravity);

    actor->table = create_table_ref();
    actor->flags = AF_DEFAULT;
    actor->userdata = create_pointer_ref("actor", actor);
    actor->tag = tag;

    actor->collision_size[0] = actor->bump_size[0] = 8;
    actor->collision_size[1] = actor->bump_size[1] = 16;
    actor->mass = 0;
    actor->bump_index = 0;
    actor->previous_bump = actor->next_bump = NULL;

    if (invoke_create) {
        if (type->create != LUA_NOREF)
            execute_ref_in(type->create, actor->userdata, type->name);
    } else {
        // Assume that this actor's create() will be invoked later on
        actor->flags |= AF_NEW;
    }

    return hid_to_actor(hid) != NULL ? actor : NULL;
}

struct ActorCamera* create_actor_camera(struct Actor* actor) {
    if (actor->camera != NULL)
        return actor->camera;

    struct ActorCamera* camera = lame_alloc(sizeof(struct ActorCamera));
    camera->actor = actor;
    camera->parent = camera->child = NULL;

    camera->targets = NULL;
    camera->pois = NULL;

    glm_vec3_copy(actor->pos, camera->pos);
    glm_vec3_copy(actor->angle, camera->angle);
    camera->fov = 45;
    camera->range = 0;

    camera->userdata = create_pointer_ref("camera", camera);
    camera->table = create_table_ref();
    camera->flags = CF_DEFAULT;

    glm_mat4_identity(camera->view_matrix);
    glm_mat4_identity(camera->projection_matrix);
    camera->surface = NULL;
    camera->surface_ref = LUA_NOREF;

    actor->camera = camera;
    if (actor->type->create_camera != LUA_NOREF)
        execute_ref_in_child(actor->type->create_camera, actor->userdata, camera->userdata, actor->type->name);

    return actor->camera;
}

struct ModelInstance* create_actor_model(struct Actor* actor, struct Model* model) {
    if (actor->model != NULL)
        return actor->model;

    struct ModelInstance* inst = create_model_instance(model);
    glm_vec3_copy(actor->pos, inst->pos);
    glm_vec3_copy(actor->angle, inst->angle);

    return (actor->model = inst);
}

void tick_actor(struct Actor* actor) {
    if (actor->type->tick != LUA_NOREF)
        execute_ref_in(actor->type->tick, actor->userdata, actor->type->name);

    struct ModelInstance* model = actor->model;
    if (model != NULL) {
        glm_vec3_copy(actor->pos, model->pos);
        glm_vec3_copy(actor->angle, model->angle);
    }

    struct ActorCamera* camera = actor->camera;
    if (camera != NULL) {
        if (camera->flags & CF_COPY_POSITION)
            glm_vec3_copy(actor->pos, camera->pos);
        if (camera->flags & CF_COPY_ANGLE)
            glm_vec3_copy(actor->angle, camera->angle);
        if (actor->type->tick_camera != LUA_NOREF)
            execute_ref_in_child(actor->type->tick_camera, actor->userdata, camera->userdata, actor->type->name);
    }
}

void destroy_actor_later(struct Actor* actor, bool natural) {
    if (actor->flags & AF_DESTROYED)
        return;
    actor->flags |= AF_DESTROYED;
    if (natural)
        actor->flags |= AF_DESTROYED_NATURAL;
}

void destroy_actor(struct Actor* actor, bool natural, bool dispose) {
    if (natural && actor->type->on_destroy != LUA_NOREF)
        execute_ref_in(actor->type->on_destroy, actor->userdata, actor->type->name);
    if (actor->type->cleanup != LUA_NOREF)
        execute_ref_in(actor->type->cleanup, actor->userdata, actor->type->name);

    if (actor->base != NULL) {
        if (dispose && ((actor->flags & AF_DISPOSABLE) || (actor->base->flags & RAF_DISPOSABLE)))
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

    if (actor->flags & AF_BUMPABLE) {
        struct Actor** chunks = actor->room->bump.chunks;
        if (chunks[actor->bump_index] == actor)
            chunks[actor->bump_index] = actor->previous_bump;
        if (actor->previous_bump != NULL)
            actor->previous_bump->next_bump = actor->next_bump;
        if (actor->next_bump != NULL)
            actor->next_bump->previous_bump = actor->previous_bump;
    }

    destroy_actor_camera(actor);
    destroy_actor_model(actor);

    if (actor->player != NULL && actor->player->actor == actor)
        actor->player->actor = NULL;

    unreference_pointer(&(actor->userdata));
    unreference(&(actor->table));
    destroy_handle(actor_handles, actor->hid);
    lame_free(&actor);
}

void destroy_actor_camera(struct Actor* actor) {
    struct ActorCamera* camera = actor->camera;
    if (camera == NULL)
        return;

    if (get_active_camera() == camera)
        set_active_camera(camera->child);
    if (camera->parent != NULL)
        camera->parent->child = camera->child;
    if (camera->child != NULL)
        camera->child->parent = camera->parent;

    struct CameraTarget* target = camera->targets;
    while (target != NULL) {
        // TODO
    }

    struct CameraPOI* poi = camera->pois;
    while (poi != NULL) {
        // TODO
    }

    if (camera->surface != NULL)
        dispose_surface(camera->surface);
    unreference(&(camera->surface_ref));

    unreference_pointer(&(camera->userdata));
    unreference(&(camera->table));
    lame_free(&(actor->camera));
}

void destroy_actor_model(struct Actor* actor) {
    CLOSE_POINTER(actor->model, destroy_model_instance);
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

void set_actor_pos(struct Actor* actor, float x, float y, float z) {
    if (actor->flags & AF_BUMPABLE) {
        // Unlink
        struct BumpMap* bump = &(actor->room->bump);
        struct Actor** chunks = bump->chunks;
        if (chunks[actor->bump_index] == actor)
            chunks[actor->bump_index] = actor->previous_bump;
        if (actor->previous_bump != NULL)
            actor->previous_bump->next_bump = actor->next_bump;
        if (actor->next_bump != NULL)
            actor->next_bump->previous_bump = actor->previous_bump;

        actor->pos[0] = x;
        actor->pos[1] = y;
        actor->pos[2] = z;

        // Link
        size_t bump_x = (x - bump->pos[0]) / BUMP_CHUNK_SIZE;
        size_t bump_y = (y - bump->pos[1]) / BUMP_CHUNK_SIZE;
        actor->bump_index =
            SDL_clamp(bump_x, 0, bump->size[0] - 1) + (bump->size[0] * SDL_clamp(bump_y, 0, bump->size[1] - 1));
        if (chunks[actor->bump_index] != NULL)
            chunks[actor->bump_index]->next_bump = actor;
        actor->previous_bump = chunks[actor->bump_index];
        actor->next_bump = NULL;
        chunks[actor->bump_index] = actor;

        return;
    }

    actor->pos[0] = x;
    actor->pos[1] = y;
    actor->pos[2] = z;
}
