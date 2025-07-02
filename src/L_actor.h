#pragma once

#include "L_math.h" // IWYU pragma: keep
#include "L_memory.h"
#include "L_player.h"

#define ACTOR_NAME_MAX 128

#include "L_room.h" // L_room.h relies on ACTOR_NAME_MAX, so only include after that's defined

typedef HandleID ActorID;

enum ActorFlags {
    AF_NONE = 0,

    AF_PERSISTENT = 1 << 0, // Will not be destroyed when deactivating a room.
    AF_DISPOSABLE = 1 << 1, // If this actor has a room and is destroyed, it will not spawn again.
    AF_UNIQUE = 1 << 2,     // Cannot be created if an instance of it already exists in the same room.

    AF_VISIBLE = 1 << 3,     // Can be drawn as long as it's not culled.
    AF_XRAY = 1 << 4,        // Will be drawn as a silhouette behind other objects.
    AF_SHADOW = 1 << 5,      // Has a blob shadow.
    AF_SHADOW_BONE = 1 << 6, // Blob shadow follows a bone instead of the actual position.

    AF_FROZEN = 1 << 7,              // Won't tick.
    AF_CULLED = 1 << 8,              // Culled in game state, won't tick.
    AF_DESTROY_WHEN_CULLED = 1 << 9, // Destroys itself as soon as it's physically culled.
};

enum CameraFlags {
    CF_NONE = 0,

    CF_THIRD_PERSON = 1 << 0, // Camera follows targets in third-person mode.
    CF_RAYCAST = 1 << 1,      // Raycast from targets to
};

struct ActorType {
    const char* name;
    struct ActorType* parent;
};

struct ActorCamera {
    ActorID parent, child;
    float fov;
    enum CameraFlags flags;

    struct CameraTarget* targets;
    struct CameraPOI* pois;
};

struct CameraTarget {
    struct CameraTarget *previous, *next;

    vec3 pos;
    ActorID target;
    float range;
};

struct CameraPOI {
    struct CameraPOI *previous, *next;

    vec3 pos;
    ActorID target;
    float lerp;
};

struct Actor {
    ActorID hid;
    struct ActorType* type;
    struct ActorCamera* acamera;

    struct Room* room;
    struct RoomActor* aroom;

    struct Player* player;
    ActorID master, target;

    vec3 pos, angle, vel;
    float friction, gravity;

    int table;
    enum ActorFlags flags;
    uint16_t tag;
};

void actor_init();
void actor_teardown();
