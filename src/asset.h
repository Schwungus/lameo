#pragma once

#include <fmod.h>
#include <yyjson.h>

#include "mem.h"

#define ASSET_NAME_MAX 128
#define JSON_FLAGS (YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS)

typedef HandleID SpriteID;
typedef HandleID MaterialID;
typedef HandleID ModelID;
typedef HandleID FontID;
typedef HandleID SoundID;
typedef HandleID TrackID;

typedef FMOD_SOUND Sample;
typedef FMOD_SOUND Stream;

struct Sprite {
    SpriteID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Sprite *previous, *next;
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

struct Font {
    FontID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Font *previous, *next;
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
