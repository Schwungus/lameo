#pragma once

// FIXME: temporary just to make this compile again......
struct Shader;
struct Texture;
struct Material;
struct Model;
struct Font;
struct Sound;
struct Track;

#include "L_math.h"
#include "L_memory.h"
#include "L_video.h" // IWYU pragma: keep

#include "L_audio.h" // IWYU pragma: keep

#define BBMOD_VERSION_MAJOR 3
#define BBMOD_VERSION_MINOR 4

#define BEGIN_ASSET(assettype, hidtype)                                                                                \
    typedef HandleID hidtype;                                                                                          \
    struct assettype {                                                                                                 \
        hidtype hid;                                                                                                   \
        const char* name;                                                                                              \
        bool transient;                                                                                                \
        int userdata;

#define END_ASSET(mapname, assetname, assettype, hidtype)                                                              \
    }                                                                                                                  \
    ;                                                                                                                  \
                                                                                                                       \
    void mapname##_init();                                                                                             \
    void mapname##_teardown();                                                                                         \
    void load_##assetname(const char*);                                                                                \
    struct assettype* fetch_##assetname(const char*);                                                                  \
    hidtype fetch_##assetname##_hid(const char*);                                                                      \
    struct assettype* get_##assetname(const char*);                                                                    \
    hidtype get_##assetname##_hid(const char*);                                                                        \
    struct assettype* hid_to_##assetname(hidtype);                                                                     \
    void destroy_##assetname(struct assettype*);                                                                       \
    void destroy_##assetname##_hid(hidtype);                                                                           \
    void clear_##mapname(bool);

#define SOURCE_ASSET(mapname, assetname, assettype, hidtype)                                                           \
    static struct HashMap* mapname = NULL;                                                                             \
    static struct Fixture* assetname##_handles = NULL;                                                                 \
                                                                                                                       \
    void mapname##_init() {                                                                                            \
        mapname = create_hash_map();                                                                                   \
        assetname##_handles = create_fixture();                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void mapname##_teardown() {                                                                                        \
        destroy_hash_map(mapname, true);                                                                               \
        CLOSE_POINTER(assetname##_handles, destroy_fixture);                                                           \
    }                                                                                                                  \
                                                                                                                       \
    assettype get_##assetname(const char* name) {                                                                      \
        return (assettype)from_hash_map(mapname, name);                                                                \
    }                                                                                                                  \
                                                                                                                       \
    hidtype get_##assetname##_hid(const char* name) {                                                                  \
        assettype asset = get_##assetname(name);                                                                       \
        return asset == NULL ? 0 : asset->hid;                                                                         \
    }                                                                                                                  \
                                                                                                                       \
    assettype fetch_##assetname(const char* name) {                                                                    \
        load_##assetname(name);                                                                                        \
        return get_##assetname(name);                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    hidtype fetch_##assetname##_hid(const char* name) {                                                                \
        load_##assetname(name);                                                                                        \
        return get_##assetname##_hid(name);                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    assettype hid_to_##assetname(hidtype hid) {                                                                        \
        return (assettype)hid_to_pointer(assetname##_handles, (HandleID)hid);                                          \
    }                                                                                                                  \
                                                                                                                       \
    void destroy_##assetname##_hid(hidtype hid) {                                                                      \
        assettype asset = (assettype)hid_to_pointer(assetname##_handles, (HandleID)hid);                               \
        if (asset != NULL)                                                                                             \
            destroy_##assetname(asset);                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    void clear_##mapname(bool teardown) {                                                                              \
        for (size_t i = 0; mapname->count > 0 && i < mapname->capacity; i++) {                                         \
            struct KeyValuePair* kvp = &mapname->items[i];                                                             \
            if (kvp->key == NULL || kvp->key == HASH_TOMBSTONE)                                                        \
                continue;                                                                                              \
            assettype asset = kvp->value;                                                                              \
            if (asset != NULL && (!asset->transient || teardown))                                                      \
                destroy_##assetname(asset);                                                                            \
        }                                                                                                              \
    }

#define ASSET_SANITY_PUSH(varname, map)                                                                                \
    if (!to_hash_map(map, varname->name, varname, false))                                                              \
        FATAL(#varname " \"%s\" already in HashMap", varname->name);

#define ASSET_SANITY_POP(varname, map)                                                                                 \
    if (varname != pop_hash_map(map, varname->name, false))                                                            \
        FATAL("Pointer mismatch when destroying " #varname " \"%s\"", varname->name);

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

BEGIN_ASSET(Shader, ShaderID)
    GLuint program;
    SDL_PropertiesID uniforms;
END_ASSET(shaders, shader, Shader, ShaderID)

BEGIN_ASSET(Texture, TextureID)
    struct Texture* parent;
    TextureID* children;
    size_t num_children;

    GLuint texture;
    uint16_t size[2], offset[2];
    GLfloat uvs[4];
END_ASSET(textures, texture, Texture, TextureID)

BEGIN_ASSET(Material, MaterialID)
    TextureID* textures[2]; // (0) Base and (1) blend textures [u_texture, u_blend_texture]
    size_t num_textures[2]; // (0) Base and (1) blend texture count
    float texture_speed[2]; // Cycle factor between (0) base and (1) blend textures per millisecond

    bool filter;            // Apply texture filtering
    GLfloat color[4];       // Multiply texture by this color [u_material_color]
    GLfloat alpha_test;     // Test texture alpha with this threshold [u_alpha_test]
    GLfloat bright;         // Ineffectiveness of light on material [u_bright]
    GLfloat scroll[2];      // Full texture scrolls per millisecond [u_scroll]
    GLfloat specular[2];    // (0) Specular factor and (1) exponent [u_specular]
    GLfloat rimlight[2];    // (0) Rimlight factor and (1) exponent [u_rimlight]
    GLboolean half_lambert; // Enable half-lambert shading on this material [u_half_lambert]
    GLfloat cel;            // Cel-shading factor [u_cel]
    GLfloat wind[3];        // (0) Wind effect factor, (1) speed and (2) resistance factor towards vertical UV origin [u_wind]
END_ASSET(materials, material, Material, MaterialID)

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

BEGIN_ASSET(Model, ModelID)
    struct Submodel* submodels;
    size_t num_submodels;

    struct Node* root_node;
    size_t num_nodes;

    DualQuaternion* bone_offsets;
    size_t num_bones;

    MaterialID* materials;
    size_t num_materials;
END_ASSET(models, model, Model, ModelID)

void destroy_node(struct Node*);

BEGIN_ASSET(Animation, AnimationID)
    size_t num_frames;
    float frame_speed;
    size_t num_nodes, num_bones;

    DualQuaternion** parent_frames;
    DualQuaternion** world_frames;
    DualQuaternion** bone_frames;
END_ASSET(animations, animation, Animation, AnimationID)

struct Glyph {
    GLfloat size[2], offset[2], uvs[4], advance;
};

BEGIN_ASSET(Font, FontID)
    TextureID texture;
    float size;
    struct Glyph** glyphs;
    size_t num_glyphs;
END_ASSET(fonts, font, Font, FontID)

BEGIN_ASSET(Sound, SoundID)
    Sample** samples;
    size_t num_samples;

    float gain;
    float pitch[2]; // lower, upper
END_ASSET(sounds, sound, Sound, SoundID)

BEGIN_ASSET(Track, TrackID)
    Stream* stream;
END_ASSET(music, track, Track, TrackID)

// clang-format on
