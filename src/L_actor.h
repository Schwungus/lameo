#pragma once

#include "L_math.h" // IWYU pragma: keep
#include "L_memory.h"
#include "L_player.h"
#include "L_script.h" // IWYU pragma: keep

#include "L_room.h" // L_room.h relies on ACTOR_NAME_MAX, so only include after that's defined

typedef HandleID ActorID;

enum ActorFlags {
    AF_NONE = 0,

    AF_PERSISTENT = 1 << 0, // Will not be destroyed when deactivating a room.
    AF_DISPOSABLE = 1 << 1, // If this actor has a base and is destroyed, it will not spawn in its own room again.
    AF_UNIQUE = 1 << 2,     // Cannot be created if an instance of it already exists in the same room.

    AF_VISIBLE = 1 << 3,     // Can be drawn as long as it's not culled.
    AF_XRAY = 1 << 4,        // Will be drawn as a silhouette behind other objects.
    AF_SHADOW = 1 << 5,      // Has a blob shadow.
    AF_SHADOW_BONE = 1 << 6, // Blob shadow follows a bone instead of the actual position.

    AF_GARBAGE = 1 << 7,             // User-specific flag. Actor is considered unimportant.
    AF_FROZEN = 1 << 8,              // Won't tick.
    AF_DESTROY_WHEN_CULLED = 1 << 9, // Destroys itself as soon as it's culled in the world.

    AF_NEW = 1 << 10,    // [INTERNAL] Actor was created but hasn't invoked `create()` yet.
    AF_CULLED = 1 << 11, // [INTERNAL] Culled in world, won't tick.

    AF_DEFAULT = AF_VISIBLE,
};

enum CameraFlags {
    CF_NONE = 0,

    CF_THIRD_PERSON = 1 << 0, // Camera follows targets in third-person mode.
    CF_RAYCAST = 1 << 1,      // Raycast from targets to
};

struct ActorType {
    const char* name;
    struct ActorType* parent;

    int load;
    int create;
    int on_destroy, cleanup;
    int tick;
    int draw, draw_screen, draw_ui;
};

// Actor component for camera functionality.
struct ActorCamera {
    ActorID parent, child;
    float fov;
    enum CameraFlags flags;

    struct CameraTarget* targets;
    struct CameraPOI* pois;
};

struct CameraTarget {
    struct CameraTarget *previous, *next; // Position in list (previous-order)

    vec3 pos;       // Exact position (relative if a target exists)
    ActorID target; // Actor to follow
    float range;    // Distance in third-person mode
};

struct CameraPOI {
    struct CameraPOI *previous, *next; // Position in list (previous-order)

    vec3 pos;       // Exact position (relative if a target exists)
    ActorID target; // Actor to track
    float lerp;     // Turn factor
};

struct Actor {
    ActorID hid;
    struct ActorType* type;
    struct ActorCamera* camera;

    struct Actor *previous, *next;                   // Position in global list (previous-order)
    struct Actor *previous_neighbor, *next_neighbor; // Position in local list (previous-order)

    struct Room* room;
    struct RoomActor* base;

    struct Player* player;
    ActorID master, target; // User-specific

    vec3 pos; // Read-only
    vec3 angle, vel;
    float friction, gravity;

    int table; // User-specific
    enum ActorFlags flags;
    uint16_t tag; // User-specific
};

void actor_init();
void actor_teardown();

int define_actor(lua_State*);
bool load_actor(const char*);
struct ActorType* get_actor_type(const char*);

struct Actor*
create_actor(struct Room*, struct RoomActor*, const char*, bool, float, float, float, float, float, float, uint16_t);
struct Actor* create_actor_from_type(
    struct Room*, struct RoomActor*, struct ActorType*, bool, float, float, float, float, float, float, uint16_t
);
void tick_actor(struct Actor*);
void draw_actor(struct Actor*);
void destroy_actor(struct Actor*, bool);

struct Actor* hid_to_actor(ActorID);
bool actor_is_ancestor(struct Actor*, const char*);
