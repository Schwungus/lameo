#pragma once

#include <fmod.h>

#include "mem.h"

#define ASSET_NAME_MAX 128

struct Sprite {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Sprite *previous, *next;
};

struct Material {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Material *previous, *next;
};

struct Model {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Model *previous, *next;
};

struct Font {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Font *previous, *next;
};

struct Sound {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Sound *previous, *next;

    FMOD_SOUND** samples;
    size_t num_samples;

    float gain, pitch[3];
};

struct Track {
    HandleID hid;
    char name[ASSET_NAME_MAX];
    bool transient;
    struct Track *previous, *next;

    FMOD_SOUND* stream;
};

void asset_init();
void asset_teardown();

void load_track(const char*);
struct Track* fetch_track(const char*);
HandleID fetch_track_hid(const char*);
struct Track* get_track(const char*);
HandleID get_track_hid(const char*);
struct Track* hid_to_track(HandleID);
void destroy_track(struct Track*);
void destroy_track_hid(HandleID);
void clear_music(int);
