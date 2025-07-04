#pragma once

#include "L_actor.h"
#include "L_level.h"
#include "L_math.h" // IWYU pragma: keep

enum RoomActorFlags {
    RAF_PERSISTENT, // Enables AF_PERSISTENT.
    RAF_DISPOSABLE, // Enables AF_DISPOSABLE.
    RAF_DISPOSED,   // Won't be spawned upon activating the room.
};

struct Room {
    struct Level* level;
    uint32_t id;

    struct Player *master, *players;

    struct RoomActor* room_actors;
    struct Actor* actors;
};

struct RoomActor {
    struct ActorType* type;
    struct RoomActor *previous, *next;

    vec3 pos, angle;
    uint16_t tag;
    enum RoomActorFlags flags;
};

void destroy_room(struct Room*);
