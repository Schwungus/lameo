#include <SDL3/SDL_stdinc.h>
#include <yyjson.h>

#include "asset.h"
#include "audio.h"
#include "log.h"
#include "mod.h"

#define ASSET_PATH_MAX 256

static struct Sprite* sprites = NULL;
static struct Fixture* sprite_handles = NULL;

static struct Material* materials = NULL;
static struct Fixture* material_handles = NULL;

static struct Model* models = NULL;
static struct Fixture* model_handles = NULL;

static struct Font* fonts = NULL;
static struct Fixture* font_handles = NULL;

static struct Sound* sounds = NULL;
static struct Fixture* sound_handles = NULL;

static struct Track* music = NULL;
static struct Fixture* track_handles = NULL;

void asset_init() {
    sprite_handles = create_fixture();
    material_handles = create_fixture();
    model_handles = create_fixture();
    font_handles = create_fixture();
    sound_handles = create_fixture();
    track_handles = create_fixture();

    INFO("Opened");
}

void asset_teardown() {
    clear_music(1);

    CLOSE_POINTER(sprite_handles, destroy_fixture);
    CLOSE_POINTER(material_handles, destroy_fixture);
    CLOSE_POINTER(model_handles, destroy_fixture);
    CLOSE_POINTER(font_handles, destroy_fixture);
    CLOSE_POINTER(sound_handles, destroy_fixture);
    CLOSE_POINTER(track_handles, destroy_fixture);

    INFO("Closed");
}

static char path_helper[ASSET_PATH_MAX];
void load_track(const char* name) {
    if (get_track(name) != NULL)
        return;

    SDL_snprintf(path_helper, sizeof(path_helper), "music/%s.*", name);
    const char* track_file = get_file(path_helper, ".json");
    if (track_file == NULL) {
        WARN("Track \"%s\" not found", name);
        return;
    }

    struct Track* track = lame_alloc(sizeof(struct Track));

    // General
    track->hid = 0;
    SDL_strlcpy(track->name, name, sizeof(track->name));
    track->transient = 0;
    if (music != NULL)
        music->next = track;
    track->previous = music;
    track->next = NULL;
    music = track;

    // Data
    load_stream(track_file, &track->stream);

    track->hid = (TrackID)create_handle(track_handles, track);

    INFO("Loaded track \"%s\" (%u)", name, track->hid);
}

struct Track* fetch_track(const char* name) {
    load_track(name);
    return get_track(name);
}

TrackID fetch_track_hid(const char* name) {
    load_track(name);
    return get_track_hid(name);
}

struct Track* get_track(const char* name) {
    for (struct Track* track = music; track != NULL; track = track->previous) {
        if (SDL_strcmp(track->name, name) == 0)
            return track;
    }

    return NULL;
}

TrackID get_track_hid(const char* name) {
    for (struct Track* track = music; track != NULL; track = track->previous) {
        if (SDL_strcmp(track->name, name) == 0)
            return track->hid;
    }

    return 0;
}

extern struct Track* hid_to_track(TrackID hid) {
    return (struct Track*)hid_to_pointer(track_handles, (HandleID)hid);
}

void destroy_track(struct Track* track) {
    if (music == track)
        music = track->previous;
    if (track->previous != NULL)
        track->previous->next = track->next;
    if (track->next != NULL)
        track->next->previous = track->previous;

    destroy_stream(track->stream);
    INFO("Freed track \"%s\" (%u)", track->name, track->hid);
    destroy_handle(track_handles, track->hid);
    lame_free(&track);
}

void destroy_track_hid(TrackID hid) {
    struct Track* track = (struct Track*)hid_to_pointer(track_handles, (HandleID)hid);
    if (track != NULL)
        destroy_track(track);
}

void clear_music(int teardown) {
    struct Track* track = music;

    while (track != NULL) {
        struct Track* it = track->previous;
        if (!track->transient || teardown)
            destroy_track(track);
        track = it;
    }
}
