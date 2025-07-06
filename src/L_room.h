#pragma once

#include "L_actor.h"
#include "L_level.h"
#include "L_math.h" // IWYU pragma: keep

enum RoomActorFlags {
    RAF_NONE = 0,

    RAF_PERSISTENT = 1 << 0, // Enables AF_PERSISTENT.
    RAF_DISPOSABLE = 1 << 1, // Enables AF_DISPOSABLE.
    RAF_DISPOSED = 1 << 2,   // Won't be spawned upon activating the room.

    RAF_DEFAULT = RAF_NONE,
};

struct Room {
    struct Level* level;
    uint32_t id;

    struct Player* master;
    struct Player* players; // List of players (previous-order)

    struct RoomActor* room_actors; // List of room actors (previous-order)
    struct Actor* actors;          // List of actors (previous-order)
};

struct RoomActor {
    struct ActorType* type;
    struct RoomActor *previous, *next; // Position in list (previous-order)
    struct Actor* actor;

    vec3 pos, angle;
    uint16_t tag;
    enum RoomActorFlags flags;
};

void destroy_room(struct Room*);
void activate_room(struct Room*);
void deactivate_room(struct Room*);
