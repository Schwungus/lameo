#pragma once

#include <fmod.h>
#include <yyjson.h>

#include "mem.h"
#include "video.h"

#define ASSET_NAME_MAX 128

typedef HandleID ShaderID;
typedef HandleID TextureID;
typedef HandleID MaterialID;
typedef HandleID ModelID;
typedef HandleID FontID;
typedef HandleID SoundID;
typedef HandleID TrackID;

typedef FMOD_SOUND Sample;
typedef FMOD_SOUND Stream;

struct Shader {
    ShaderID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Shader *previous, *next;

    GLuint program;
    SDL_PropertiesID uniforms;
};

struct Texture {
    TextureID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Texture *previous, *next;

    struct Texture* parent;
    TextureID* children;
    size_t num_children;

    GLuint texture;
    uint16_t size[2], offset[2];
    GLfloat uvs[4];
};

struct Material {
    MaterialID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Material *previous, *next;
};

struct Model {
    ModelID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Model *previous, *next;
};

struct Glyph {
    GLfloat size[2], offset[2], uvs[4], advance;
};

struct Font {
    FontID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Font *previous, *next;

    TextureID texture;
    float size;
    struct Glyph* glyphs;
    size_t num_glyphs;
};

struct Sound {
    SoundID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Sound *previous, *next;

    Sample** samples;
    size_t num_samples;

    float gain;
    float pitch[2]; // lower, upper
};

struct Track {
    TrackID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Track *previous, *next;

    Stream* stream;
};

void asset_init();
void asset_teardown();

inline void clear_assets(int);

void load_shader(const char*);
struct Shader* fetch_shader(const char*);
ShaderID fetch_shader_hid(const char*);
struct Shader* get_shader(const char*);
ShaderID get_shader_hid(const char*);
inline struct Shader* hid_to_shader(ShaderID);
void destroy_shader(struct Shader*);
void destroy_shader_hid(ShaderID);
void clear_shaders(int);

void load_texture(const char*);
struct Texture* fetch_texture(const char*);
TextureID fetch_texture_hid(const char*);
struct Texture* get_texture(const char*);
TextureID get_texture_hid(const char*);
inline struct Texture* hid_to_texture(TextureID);
void destroy_texture(struct Texture*);
void destroy_texture_hid(TextureID);
void clear_textures(int);

void load_font(const char*);
struct Font* fetch_font(const char*);
FontID fetch_font_hid(const char*);
struct Font* get_font(const char*);
FontID get_font_hid(const char*);
inline struct Font* hid_to_font(FontID);
void destroy_font(struct Font*);
void destroy_font_hid(FontID);
void clear_fonts(int);

struct Sound* create_sound(const char*);
void load_sound(const char*);
struct Sound* fetch_sound(const char*);
SoundID fetch_sound_hid(const char*);
struct Sound* get_sound(const char*);
SoundID get_sound_hid(const char*);
inline struct Sound* hid_to_sound(SoundID);
void destroy_sound(struct Sound*);
void destroy_sound_hid(SoundID);
void clear_sounds(int);

void load_track(const char*);
struct Track* fetch_track(const char*);
TrackID fetch_track_hid(const char*);
struct Track* get_track(const char*);
TrackID get_track_hid(const char*);
inline struct Track* hid_to_track(TrackID);
void destroy_track(struct Track*);
void destroy_track_hid(TrackID);
void clear_music(int);
