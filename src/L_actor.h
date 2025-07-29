#pragma once

#include "L_asset.h"
#include "L_math.h" // IWYU pragma: keep
#include "L_memory.h"
#include "L_player.h"
#include "L_room.h"
#include "L_script.h" // IWYU pragma: keep
#include "L_video.h"

typedef HandleID ActorID;

enum ActorFlags {
    AF_NONE = 0,

    // Room
    AF_PERSISTENT = 1 << 0, // Will not be destroyed when deactivating a room.
    AF_DISPOSABLE = 1 << 1, // If this actor has a base and is destroyed, it will not spawn in its own room again.
    AF_UNIQUE = 1 << 2,     // [UNUSED] Deal with it your god damn self!

    // Visual
    AF_VISIBLE = 1 << 3,     // Can be drawn as long as it's not culled.
    AF_XRAY = 1 << 4,        // Will be drawn as a silhouette behind other objects.
    AF_SHADOW = 1 << 5,      // Has a blob shadow.
    AF_SHADOW_BONE = 1 << 6, // Blob shadow follows a bone instead of the actual position.

    // Handling
    AF_GARBAGE = 1 << 7,             // User-specific flag. Actor is considered unimportant.
    AF_FROZEN = 1 << 8,              // Won't tick.
    AF_DESTROY_WHEN_CULLED = 1 << 9, // Destroys itself as soon as it's culled in the world.

    // Internal
    AF_NEW = 1 << 10,               // [INTERNAL] Was created, but hasn't invoked `create()` yet.
    AF_CULLED = 1 << 11,            // [INTERNAL] Culled in world, won't tick.
    AF_DESTROYED = 1 << 12,         // [INTERNAL] Marked for deletion.
    AF_DESTROYED_NATURAL = 1 << 13, // [INTERNAL] Will invoke `on_destroy()`.

    // Collision
    AF_COLLISION = 1 << 14, // [INTERNAL] Collides with colliders.
    AF_BUMP = 1 << 15,      // [INTERNAL] Bumps other actors.
    AF_BUMPABLE = 1 << 16,  // [INTERNAL] Bumped by other actors.
    AF_PUSH = 1 << 17,      // [INTERNAL] Pushes actors with lower or equal mass.
    AF_PUSHABLE = 1 << 18,  // [INTERNAL] Pushed by other actors with greater or equal mass.
    AF_HITSCAN = 1 << 19,   // Intercepts hitscans.

    AF_DEFAULT = AF_VISIBLE,
};

enum CameraFlags {
    CF_NONE = 0,

    CF_ORTHOGONAL = 1 << 0,    // Use orthogonal view instead of perspective
    CF_THIRD_PERSON = 1 << 1,  // Camera follows targets in third-person mode.
    CF_RAYCAST = 1 << 2,       // Stick to collision meshes while following targets.
    CF_COPY_POSITION = 1 << 3, // Copy source position.
    CF_COPY_ANGLE = 1 << 4,    // Copy source angle.

    CF_DEFAULT = CF_COPY_POSITION | CF_COPY_ANGLE,
};

struct ActorType {
    const char* name;
    struct ActorType* parent;

    int load;
    int create, create_camera;
    int on_destroy, cleanup;
    int tick, tick_camera;
    int draw, draw_screen, draw_ui;
};

// Actor component for camera functionality.
struct ActorCamera {
    struct Actor* actor;
    struct ActorCamera *parent, *child;
    int userdata;

    struct CameraTarget* targets;
    struct CameraPOI* pois;

    vec3 pos, angle;
    float fov, range;

    int table; // User-specific
    enum CameraFlags flags;

    mat4 view_matrix, projection_matrix;
    struct Surface* surface;
    int surface_ref;
};

struct CameraTarget {
    struct ActorCamera* camera;
    struct CameraTarget *previous, *next; // Position in list (previous-order)
    int userdata;

    vec3 pos;       // Exact position (relative if a target exists)
    ActorID target; // Actor to follow
    float range;    // Distance in third-person mode
};

struct CameraPOI {
    struct ActorCamera* camera;
    struct CameraPOI *previous, *next; // Position in list (previous-order)
    int userdata;

    vec3 pos;       // Exact position (relative if a target exists)
    ActorID target; // Actor to track
    float lerp;     // Turn factor
};

struct Actor {
    ActorID hid;
    struct ActorType* type;
    struct ActorCamera* camera;
    struct ModelInstance* model;
    int userdata;

    struct Actor *previous, *next;                   // Position in global list (previous-order)
    struct Actor *previous_neighbor, *next_neighbor; // Position in local list (previous-order)

    struct Room* room;
    struct RoomActor* base;

    struct Player* player;

    vec3 pos;   // Read-only
    vec3 angle; // (0) Yaw, (1) pitch and (2) roll
    vec3 vel, friction, gravity;

    int table; // User-specific
    enum ActorFlags flags;
    uint16_t tag; // User-specific

    vec2 collision_size, bump_size;          // (0) Radius and (1) height
    float mass;                              // Push priority
    size_t bump_index;                       // Cell index in bump map
    struct Actor *previous_bump, *next_bump; // Position in bump list (previous-order)

    FMOD_CHANNELGROUP* emitter;
    FMOD_CHANNEL* voice;
};

void actor_init();
void actor_teardown();

int define_actor(lua_State*);
bool load_actor(const char*);
struct ActorType* get_actor_type(const char*);
struct Actor* get_actors();

struct Actor*
create_actor(struct Room*, struct RoomActor*, const char*, bool, float, float, float, float, float, float, uint16_t);
struct Actor* create_actor_from_type(
    struct Room*, struct RoomActor*, struct ActorType*, bool, float, float, float, float, float, float, uint16_t
);
struct ActorCamera* create_actor_camera(struct Actor*);
struct ModelInstance* create_actor_model(struct Actor*, struct Model*);
void actor_to_sky(struct Actor*);
void tick_actor(struct Actor*);
void draw_actor(struct Actor*);
void destroy_actor_later(struct Actor*, bool);
void destroy_actor(struct Actor*, bool, bool);
void destroy_actor_camera(struct Actor*);
void destroy_actor_model(struct Actor*);

struct Actor* hid_to_actor(ActorID);
bool actor_is_ancestor(struct Actor*, const char*);

void set_actor_pos(struct Actor*, float, float, float);
