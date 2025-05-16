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
    clear_sounds(1);
    clear_music(1);

    destroy_fixture(sprite_handles);
    destroy_fixture(material_handles);
    destroy_fixture(model_handles);
    destroy_fixture(font_handles);
    destroy_fixture(sound_handles);
    destroy_fixture(track_handles);

    INFO("Closed");
}

static char path_helper[ASSET_PATH_MAX];

// Sounds
void load_sound(const char* name) {
    if (get_sound(name) != NULL)
        return;

    // Find a JSON for definitions
    SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.json", name);
    const char* file = get_file(path_helper, NULL);
    if (file == NULL) {
        // Fall back to just using a sample
        SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", name);
        if ((file = get_file(path_helper, ".json")) == NULL) {
            WARN("Sound \"%s\" not found", name);
            return;
        }

        struct Sound* sound = lame_alloc(sizeof(struct Sound));

        // General
        sound->hid = (SoundID)create_handle(sound_handles, sound);
        SDL_strlcpy(sound->name, name, sizeof(sound->name));
        sound->transient = 0;
        if (sounds != NULL)
            sounds->next = sound;
        sound->previous = sounds;
        sound->next = NULL;
        sounds = sound;

        // Data
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;
        load_sample(file, &sound->samples[0]);
        sound->gain = 1;
        sound->pitch[0] = sound->pitch[1] = 1;

        INFO("Loaded sound \"%s\" (%u)", name, sound->hid);
        return;
    }

    yyjson_doc* json = yyjson_read_file(file, JSON_FLAGS, NULL, NULL);
    if (json == NULL) {
        ERROR("Failed to load Sound \"%s.json\"", name);
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root)) {
        ERROR("Expected root object in \"%s.json\", got %s", name, yyjson_get_type_desc(root));
        yyjson_doc_free(json);
        return;
    }

    // Allocate empty sound at this point
    struct Sound* sound = lame_alloc(sizeof(struct Sound));

    // General
    sound->hid = (SoundID)create_handle(sound_handles, sound);
    SDL_strlcpy(sound->name, name, sizeof(sound->name));
    sound->transient = 0;
    if (sounds != NULL)
        sounds->next = sound;
    sound->previous = sounds;
    sound->next = NULL;
    sounds = sound;

    // Data
    sound->samples = NULL;
    sound->num_samples = 0;
    sound->gain = 1;
    sound->pitch[0] = sound->pitch[1] = 1;

    // Samples
    yyjson_val* value = yyjson_obj_get(root, "sample");
    if (yyjson_is_str(value)) {
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;

        SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", yyjson_get_str(value));
        if ((file = get_file(path_helper, ".json")) == NULL) {
            sound->samples[0] = NULL;
            WARN("Sample \"%s\" not found", path_helper);
        } else {
            load_sample(file, &sound->samples[0]);
        }
    } else if (yyjson_is_arr(value)) {
        sound->num_samples = yyjson_arr_size(value);
        sound->samples = lame_alloc(sound->num_samples * sizeof(struct Sample*));

        size_t i, n;
        yyjson_val* entry;
        yyjson_arr_foreach(value, i, n, entry) {
            if (yyjson_is_null(entry)) {
                sound->samples[i] = NULL;
                continue;
            } else if (!yyjson_is_str(entry)) {
                sound->samples[i] = NULL;
                ERROR("Expected \"sample\" index %u as string or null, got %s", i, yyjson_get_type_desc(entry));
                continue;
            }

            SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", yyjson_get_str(entry));
            if ((file = get_file(path_helper, ".json")) == NULL) {
                sound->samples[i] = NULL;
                WARN("Sample \"%s\" not found", path_helper);
                continue;
            }
            load_sample(file, &sound->samples[i]);
        }
    } else if (!yyjson_is_null(value)) {
        ERROR("Expected \"sample\" as string, array or null in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Gain
    if (yyjson_is_num(value = yyjson_obj_get(root, "gain"))) {
        sound->gain = (float)yyjson_get_num(value);
    }

    // Pitch
    value = yyjson_obj_get(root, "pitch");
    if (yyjson_is_arr(value)) {
        if (yyjson_arr_size(value) >= 2) {
            sound->pitch[0] = yyjson_get_num(yyjson_arr_get(value, 0));
            sound->pitch[1] = yyjson_get_num(yyjson_arr_get(value, 1));
        }
    } else if (yyjson_is_num(value)) {
        sound->pitch[0] = sound->pitch[1] = yyjson_get_num(value);
    }

    yyjson_doc_free(json);

    INFO("Loaded sound \"%s\" (%u)", name, sound->hid);
}

struct Sound* fetch_sound(const char* name) {
    load_sound(name);
    return get_sound(name);
}

SoundID fetch_sound_hid(const char* name) {
    load_sound(name);
    return get_sound_hid(name);
}

struct Sound* get_sound(const char* name) {
    for (struct Sound* sound = sounds; sound != NULL; sound = sound->previous)
        if (SDL_strcmp(sound->name, name) == 0)
            return sound;
    return NULL;
}

SoundID get_sound_hid(const char* name) {
    for (struct Sound* sound = sounds; sound != NULL; sound = sound->previous)
        if (SDL_strcmp(sound->name, name) == 0)
            return sound->hid;
    return 0;
}

extern struct Sound* hid_to_sound(SoundID hid) {
    return (struct Sound*)hid_to_pointer(sound_handles, (HandleID)hid);
}

void destroy_sound(struct Sound* sound) {
    if (sounds == sound)
        sounds = sound->previous;
    if (sound->previous != NULL)
        sound->previous->next = sound->next;
    if (sound->next != NULL)
        sound->next->previous = sound->previous;

    if (sound->samples != NULL) {
        for (size_t i = 0; i < sound->num_samples; i++)
            if (sound->samples[i] != NULL)
                destroy_sample(sound->samples[i]);
        lame_free(&sound->samples);
    }

    INFO("Freed sound \"%s\" (%u)", sound->name, sound->hid);
    destroy_handle(sound_handles, sound->hid);
    lame_free(&sound);
}

void destroy_sound_hid(SoundID hid) {
    struct Sound* sound = (struct Sound*)hid_to_pointer(sound_handles, (HandleID)hid);
    if (sound != NULL)
        destroy_sound(sound);
}

void clear_sounds(int teardown) {
    struct Sound* sound = sounds;

    while (sound != NULL) {
        struct Sound* it = sound->previous;
        if (!sound->transient || teardown)
            destroy_sound(sound);
        sound = it;
    }
}

// Music
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
    track->hid = (TrackID)create_handle(track_handles, track);
    SDL_strlcpy(track->name, name, sizeof(track->name));
    track->transient = 0;
    if (music != NULL)
        music->next = track;
    track->previous = music;
    track->next = NULL;
    music = track;

    // Data
    load_stream(track_file, &track->stream);

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
    for (struct Track* track = music; track != NULL; track = track->previous)
        if (SDL_strcmp(track->name, name) == 0)
            return track;
    return NULL;
}

TrackID get_track_hid(const char* name) {
    for (struct Track* track = music; track != NULL; track = track->previous)
        if (SDL_strcmp(track->name, name) == 0)
            return track->hid;
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
