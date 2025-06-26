#pragma once

#include "actor.h"
#include "level.h"
#include "math.h" // IWYU pragma: keep

enum RoomActorFlags {
    RAF_PERSISTENT, // Enables AF_PERSISTENT.
    RAF_DISPOSABLE, // Enables AF_DISPOSABLE.
    RAF_DISPOSED,   // Won't be spawned upon activating the room.
};

struct Room {
    struct Level* level;
    uint16_t id;
    struct Room *previous, *next;

    struct Player *master, *players;
    struct Actor* actors;
};

struct RoomActor {
    char type[ACTOR_NAME_MAX];
    struct RoomActor *previous, *next;

    vec3 pos, angle;
    uint16_t tag;
    enum RoomActorFlags flags;
};
