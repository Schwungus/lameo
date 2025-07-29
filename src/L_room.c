#include "L_room.h"

void update_bump_map(struct BumpMap* bump, vec2 pos) {
    if (bump->chunks == NULL) {
        bump->chunks = (struct Actor**)lame_alloc_clean(sizeof(struct Actor*));
        glm_vec2_copy(pos, bump->pos);
        bump->size[0] = bump->size[1] = 1;
        return;
    }

    bool update = false;
    float old_x1 = bump->pos[0];
    float old_y1 = bump->pos[1];
    float old_x2 = bump->pos[0] + (float)(bump->size[0] * BUMP_CHUNK_SIZE);
    float old_y2 = bump->pos[1] + (float)(bump->size[1] * BUMP_CHUNK_SIZE);

    if (pos[0] < old_x1) {
        bump->pos[0] = pos[0];
        bump->size[0] = (size_t)SDL_ceil((old_x2 - pos[0]) / (float)BUMP_CHUNK_SIZE);
        update = true;
    } else if (pos[0] > old_x2) {
        bump->size[0] += (size_t)SDL_ceil((pos[0] - old_x2) / (float)BUMP_CHUNK_SIZE);
        update = true;
    }

    if (pos[1] < old_y1) {
        bump->pos[1] = pos[1];
        bump->size[1] = (size_t)SDL_ceil((old_y2 - pos[1]) / (float)BUMP_CHUNK_SIZE);
        update = true;
    } else if (pos[1] > old_y2) {
        bump->size[1] += (size_t)SDL_ceil((pos[1] - old_y2) / (float)BUMP_CHUNK_SIZE);
        update = true;
    }

    if (update) {
        const size_t size = bump->size[0] * bump->size[1] * sizeof(struct Actor*);
        lame_realloc(&bump->chunks, size);
        lame_set(bump->chunks, 0, size);
    }
}

void destroy_room(struct Room* room) {
    struct Player* player = room->players;
    while (player != NULL) {
        player_leave_room(player);
        player = room->players;
    }

    while (room->actors != NULL)
        destroy_actor(room->actors, false, false);

    struct RoomActor* it = room->room_actors;
    while (it != NULL) {
        struct RoomActor* next = it->previous;
        unreference(&(it->special));
        lame_free(&it);
        it = next;
    }

    if (room->bump.chunks != NULL)
        lame_free(&(room->bump.chunks));

    CLOSE_POINTER(room->model, destroy_model_instance);
    CLOSE_POINTER(room->sounds, destroy_world_sound_pool);

    lame_free(&room);
}

void activate_room(struct Room* room) {
    // Pass 1: Dispatch room actors
    struct RoomActor* it = room->room_actors;
    while (it != NULL) {
        if (!(it->flags & RAF_DISPOSED) && it->actor == NULL) {
            // Create actor without invoking create(). Some actors rely on
            // other actors existing during creation.
            struct Actor* actor = create_actor_from_type(
                room, it, it->type, false, it->pos[0], it->pos[1], it->pos[2], it->angle[0], it->angle[1], it->angle[2],
                it->tag
            );
            if (actor != NULL) {
                if (it->flags & RAF_PERSISTENT)
                    actor->flags |= AF_PERSISTENT;
                if (it->flags & RAF_DISPOSABLE)
                    actor->flags |= AF_DISPOSABLE;
                actor->flags |= AF_NEW; // Mark for invocation
                it->actor = actor;
            }
        }
        it = it->previous;
    }

    // Pass 2: Invoke new actors
    it = room->room_actors;
    while (it != NULL) {
        if (it->actor != NULL && (it->actor->flags & AF_NEW)) {
            if (it->type->create != LUA_NOREF)
                execute_ref_in(it->type->create, it->actor->userdata, it->type->name);
            it->actor->flags &= ~AF_NEW;
        }
        it = it->previous;
    }
}

void deactivate_room(struct Room* room) {
    struct Actor* it = room->actors;
    while (it != NULL) {
        struct Actor* next = it->previous_neighbor;
        if (!(it->flags & AF_PERSISTENT) && (it->base == NULL || !(it->base->flags & RAF_PERSISTENT)))
            destroy_actor(it, false, false);
        it = next;
    }

    FMOD_ChannelGroup_Stop(room->sounds);
}
