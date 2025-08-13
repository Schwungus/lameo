#pragma once

// FIXME: temporary just to make this compile again......
struct Shader;
struct Texture;
struct Material;
struct Model;
struct Animation;
struct Font;
struct Sound;
struct Track;

#include "L_math.h"
#include "L_memory.h"
#include "L_video.h" // IWYU pragma: keep

#include "L_audio.h" // IWYU pragma: keep

#define BBMOD_VERSION_MAJOR 3
#define BBMOD_VERSION_MINOR 4

#define BEGIN_ASSET(assettype)                                                                                         \
    struct assettype {                                                                                                 \
        const char* name;                                                                                              \
        bool transient;                                                                                                \
        int userdata;

#define END_ASSET(mapname, assetname, assettype)                                                                       \
    }                                                                                                                  \
    ;                                                                                                                  \
                                                                                                                       \
    void mapname##_init();                                                                                             \
    void mapname##_teardown();                                                                                         \
    void load_##assetname(const char*);                                                                                \
    struct assettype* fetch_##assetname(const char*);                                                                  \
    struct assettype* get_##assetname(const char*);                                                                    \
    void destroy_##assetname(struct assettype*);                                                                       \
    void clear_##mapname(bool);

#define SOURCE_ASSET(mapname, assetname, assettype)                                                                    \
    static struct HashMap* mapname = NULL;                                                                             \
                                                                                                                       \
    void mapname##_init() {                                                                                            \
        (mapname) = create_hash_map();                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    void mapname##_teardown() {                                                                                        \
        destroy_hash_map(mapname, true);                                                                               \
    }                                                                                                                  \
                                                                                                                       \
    assettype get_##assetname(const char* name) {                                                                      \
        return (assettype)from_hash_map(mapname, name);                                                                \
    }                                                                                                                  \
                                                                                                                       \
    assettype fetch_##assetname(const char* name) {                                                                    \
        load_##assetname(name);                                                                                        \
        return get_##assetname(name);                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    void clear_##mapname(bool teardown) {                                                                              \
        for (size_t i = 0; (mapname)->count > 0 && i < (mapname)->capacity; i++) {                                     \
            struct KeyValuePair* kvp = &(mapname)->items[i];                                                           \
            if (kvp->key == NULL || kvp->key == HASH_TOMBSTONE)                                                        \
                continue;                                                                                              \
            assettype asset = kvp->value;                                                                              \
            if (asset != NULL && (!asset->transient || teardown))                                                      \
                destroy_##assetname(asset);                                                                            \
        }                                                                                                              \
    }

#define ASSET_SANITY_PUSH(varname, map)                                                                                \
    if (!to_hash_map(map, (varname)->name, varname, false))                                                            \
        FATAL(#varname " \"%s\" already in HashMap", (varname)->name);

#define ASSET_SANITY_POP(varname, map)                                                                                 \
    if ((varname) != pop_hash_map(map, (varname)->name, false))                                                        \
        FATAL("Pointer mismatch when destroying " #varname " \"%s\"", (varname)->name);

void asset_init();
void asset_teardown();

void clear_assets(bool);

typedef FMOD_SOUND Sample;
typedef FMOD_SOUND Stream;

enum BoneSpaces {
    BS_PARENT = 1 << 0,
    BS_WORLD = 1 << 1,
    BS_BONE = 1 << 2,
};

// manual formatting.....
// clang-format off

BEGIN_ASSET(Shader)
    GLuint program;
    SDL_PropertiesID uniforms;
END_ASSET(shaders, shader, Shader)

BEGIN_ASSET(Texture)
    struct Texture* parent;

    GLuint texture;
    uint16_t size[2];
    vec2 offset;
    vec4 uvs;
END_ASSET(textures, texture, Texture)

BEGIN_ASSET(Material)
    struct Texture** textures[2]; // (0) Base and (1) blend textures [u_texture, u_blend_texture]
    size_t num_textures[2];       // (0) Base and (1) blend texture count
    float texture_speed[2];       // Cycle factor between (0) base and (1) blend textures per millisecond

    bool filter;       // Apply texture filtering
    vec4 color;        // Multiply texture by this color [u_material_color]
    float alpha_test;  // Test texture alpha with this threshold [u_alpha_test]
    float bright;      // Ineffectiveness of light on material [u_bright]
    vec2 scroll;       // Full texture scrolls per millisecond [u_scroll]
    vec4 specular;     // (0) Specular factor and (1) exponent, (2) rimlight factor and (3) exponent [u_specular]
    bool half_lambert; // Enable half-lambert shading on this material [u_half_lambert]
    float cel;         // Cel-shading factor [u_cel]
    vec3 wind;         // (0) Wind effect factor, (1) speed and (2) resistance factor towards vertical UV origin [u_wind]
END_ASSET(materials, material, Material)

struct Submodel {
    GLuint vao, vbo;
    struct WorldVertex* vertices;
    size_t num_vertices;

    size_t material;
};

struct Node {
    const char* name;
    size_t index;

    struct Node* parent;
    struct Node** children;
    size_t num_children;

    bool bone;
    DualQuaternion dq;
};

BEGIN_ASSET(Model)
    struct Submodel* submodels;
    size_t num_submodels;

    struct Node* root_node;
    size_t num_nodes;

    DualQuaternion* bone_offsets;
    size_t num_bones;

    struct Material** materials;
    size_t num_materials;

    struct Texture* lightmap;
END_ASSET(models, model, Model)

void destroy_node(struct Node*);

BEGIN_ASSET(Animation)
    size_t num_frames;
    float frame_speed;

    size_t num_nodes, num_bones;
    DualQuaternion** parent_frames;
    DualQuaternion** world_frames;
    DualQuaternion** bone_frames;
END_ASSET(animations, animation, Animation)

struct Glyph {
    GLfloat size[2], offset[2], uvs[4], advance;
};

BEGIN_ASSET(Font)
    struct Texture* texture;
    float size;
    struct Glyph** glyphs;
    size_t num_glyphs;
END_ASSET(fonts, font, Font)

BEGIN_ASSET(Sound)
    Sample** samples;
    size_t num_samples;

    float gain;
    float pitch[2]; // lower, upper
END_ASSET(sounds, sound, Sound)

BEGIN_ASSET(Track)
    Stream* stream;
END_ASSET(music, track, Track)

// clang-format on
