#pragma once

#include "math.h"
#include "mem.h"
#include "player.h"

#define ACTOR_NAME_MAX 32

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
    char name[ACTOR_NAME_MAX];
    struct ActorType* parent;
};

struct CameraActor {
    ActorID parent, child;
    float fov;
    enum CameraFlags flags;
};

struct CameraTarget {
    vec3 pos;
    ActorID target;
    float range;
};

struct CameraPOI {
    vec3 pos;
    ActorID target;
    float lerp;
};

struct Actor {
    struct ActorType* type;
    struct CameraActor* acamera;

    struct Player* player;
    ActorID *master, *target;

    vec3 pos, angle, vel;
    float friction, gravity;

    SDL_PropertiesID properties;
    enum ActorFlags flags;
};
