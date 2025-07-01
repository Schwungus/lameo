#pragma once

#include "L_memory.h"
#include "L_video.h" // IWYU pragma: keep

#include "L_audio.h" // IWYU pragma: keep

#define ASSET_NAME_MAX 128

#define HEADER_ASSET(mapname, assetname, assettype, hidtype)                                                           \
    typedef HandleID hidtype;                                                                                          \
                                                                                                                       \
    void mapname##_init();                                                                                             \
    void mapname##_teardown();                                                                                         \
    void load_##assetname(const char*);                                                                                \
    assettype fetch_##assetname(const char*);                                                                          \
    hidtype fetch_##assetname##_hid(const char*);                                                                      \
    assettype get_##assetname(const char*);                                                                            \
    hidtype get_##assetname##_hid(const char*);                                                                        \
    assettype hid_to_##assetname(hidtype);                                                                             \
    void destroy_##assetname(assettype);                                                                               \
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
        for (size_t i = 0; i < mapname->capacity; i++) {                                                               \
            struct KeyValuePair* kvp = &mapname->items[i];                                                             \
            if (kvp->key == NULL)                                                                                      \
                continue;                                                                                              \
            assettype asset = kvp->value;                                                                              \
            if (asset != NULL && (!asset->transient || teardown)) {                                                    \
                destroy_##assetname(asset);                                                                            \
                kvp->value = NULL;                                                                                     \
                lame_free(&kvp->key);                                                                                  \
            }                                                                                                          \
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

HEADER_ASSET(shaders, shader, struct Shader*, ShaderID);
HEADER_ASSET(textures, texture, struct Texture*, TextureID);
HEADER_ASSET(materials, material, struct Material*, MaterialID);
HEADER_ASSET(models, model, struct Model*, ModelID);
HEADER_ASSET(fonts, font, struct Font*, FontID);
HEADER_ASSET(sounds, sound, struct Sound*, SoundID);
HEADER_ASSET(music, track, struct Track*, TrackID);

struct Shader {
    ShaderID hid;
    const char* name;
    bool transient;

    GLuint program;
    SDL_PropertiesID uniforms;
};

struct Texture {
    TextureID hid;
    const char* name;
    bool transient;

    struct Texture* parent;
    TextureID* children;
    size_t num_children;

    GLuint texture;
    uint16_t size[2], offset[2];
    GLfloat uvs[4];
};

struct Material {
    MaterialID hid;
    const char* name;
    bool transient;

    TextureID* textures[2]; // (0) Base and (1) blend textures
    size_t num_textures[2]; // (0) Base and (1) blend texture count
    float texture_speed[2]; // Cycle factor between (0) base and (1) blend textures per millisecond

    GLfloat color[4];       // Multiply texture by this color
    GLfloat alpha_test;     // Test texture alpha with this threshold
    GLfloat bright;         // Ineffectiveness of light on material
    GLfloat scroll[2];      // Full texture scrolls per millisecond
    GLfloat specular[2];    // (0) Specular factor and (1) exponent
    GLfloat rimlight[2];    // (0) Rimlight factor and (1) exponent
    GLboolean half_lambert; // Enable half-lambert shading on this material
    GLfloat cel;            // Cel-shading factor
    GLfloat wind[3];        // (0) Wind effect factor, (1) speed and (2) resistance factor towards vertical UV origin
};

struct Model {
    ModelID hid;
    const char* name;
    bool transient;
};

struct Glyph {
    GLfloat size[2], offset[2], uvs[4], advance;
};

struct Font {
    FontID hid;
    const char* name;
    bool transient;

    TextureID texture;
    float size;
    struct Glyph** glyphs;
    size_t num_glyphs;
};

typedef FMOD_SOUND Sample;
typedef FMOD_SOUND Stream;

struct Sound {
    SoundID hid;
    const char* name;
    bool transient;

    Sample** samples;
    size_t num_samples;

    float gain;
    float pitch[2]; // lower, upper
};

struct Track {
    TrackID hid;
    const char* name;
    bool transient;

    Stream* stream;
};
