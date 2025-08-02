#pragma once

#define RL_ARGS 6 // Used by L_actor.h

#include "L_actor.h"
#include "L_audio.h" // IWYU pragma: keep
#include "L_level.h"
#include "L_math.h" // IWYU pragma: keep

#define BUMP_CHUNK_SIZE 128

#define MAX_ROOM_LIGHTS 8

#define RL_INVALID 0
#define RL_SUN 1
#define RL_POINT 2
#define RL_SPOT 3

#define RL_SUN_NX 0
#define RL_SUN_NY 1
#define RL_SUN_NZ 2

#define RL_POINT_NEAR 0
#define RL_POINT_FAR 1

#define RL_SPOT_NX 0
#define RL_SPOT_NY 1
#define RL_SPOT_NZ 2
#define RL_SPOT_RANGE 3
#define RL_SPOT_IN 4
#define RL_SPOT_OUT 5

enum RoomActorFlags {
    RAF_NONE = 0,

    RAF_PERSISTENT = 1 << 0, // Enables AF_PERSISTENT.
    RAF_DISPOSABLE = 1 << 1, // Enables AF_DISPOSABLE.
    RAF_DISPOSED = 1 << 2,   // Won't be spawned upon activating the room.

    RAF_DEFAULT = RAF_NONE,
};

struct BumpMap {
    struct Actor** chunks;
    vec2 pos;
    size_t size[2];
};

struct RoomLight {
    GLfloat type;
    GLfloat pos[3];
    GLfloat color[4];
    GLfloat args[RL_ARGS];
};

struct Room {
    struct Level* level;
    uint32_t id;
    int userdata;

    struct Player* master;
    struct Player* players; // List of players (previous-order)

    struct RoomActor* room_actors; // List of room actors (previous-order)
    struct Actor* actors;          // List of actors (previous-order)
    struct BumpMap bump;

    struct ModelInstance* model;
    struct Actor* sky;
    FMOD_CHANNELGROUP* sounds;

    vec4 ambient;
    struct RoomLight lights[MAX_ROOM_LIGHTS];
    struct ActorLight* light_occupied[MAX_ROOM_LIGHTS];
    vec2 fog_distance;
    vec4 fog_color;
    vec4 wind; // (0-2) Wind direction and (3) factor
};

struct RoomActor {
    struct ActorType* type;
    struct RoomActor *previous, *next; // Position in list (previous-order)
    struct Actor* actor;

    vec3 pos, angle;
    uint16_t tag;
    enum RoomActorFlags flags;
    int special;
};

void update_bump_map(struct BumpMap*, vec2);

void destroy_room(struct Room*);
void activate_room(struct Room*);
void deactivate_room(struct Room*);
