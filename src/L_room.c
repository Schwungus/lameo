#include "L_room.h"

void destroy_room(struct Room* room) {
    struct Player* player = room->players;
    while (player != NULL) {
        player_leave_room(player);
        player = room->players;
    }

    while (room->actors != NULL)
        destroy_actor(room->actors, false);

    struct RoomActor* it = room->room_actors;
    while (it != NULL) {
        struct RoomActor* next = it->previous;
        lame_free(&it);
        it = next;
    }

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
        if (!(it->flags & AF_PERSISTENT) && (it->base == NULL || !(it->base->flags & RAF_PERSISTENT)))
            destroy_actor(it, false);
        it = (it == room->actors) ? it->previous : room->actors;
    }
}
