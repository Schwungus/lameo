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

    FMOD_SOUND** samples;
    size_t num_samples;

    float gain, pitch[3];
};

struct Track {
    TrackID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Track *previous, *next;

    FMOD_SOUND* stream;
};

void asset_init();
void asset_teardown();

void load_track(const char*);
struct Track* fetch_track(const char*);
TrackID fetch_track_hid(const char*);
struct Track* get_track(const char*);
TrackID get_track_hid(const char*);
inline struct Track* hid_to_track(TrackID);
void destroy_track(struct Track*);
void destroy_track_hid(TrackID);
void clear_music(int);
