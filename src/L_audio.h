#pragma once

#include <fmod.h>

#include "L_asset.h"

enum VolumeSlots {
    VOL_MAIN,
    VOL_FADE,
    VOL_GAME,
    VOL_SIZE,
};

struct MusicInstance {
    FMOD_CHANNEL* channel;
    uint32_t priority;
};

void audio_init();
void audio_update();
void audio_teardown();

FMOD_RESULT audio_debug_callback(FMOD_DEBUG_FLAGS, const char*, int, const char*, const char*);

void load_sample(const char*, FMOD_SOUND**);
void destroy_sample(FMOD_SOUND*);

FMOD_CHANNEL* play_ui_sound(const struct Sound*, bool, uint32_t, float, float);

void load_stream(const char*, FMOD_SOUND**);
void destroy_stream(FMOD_SOUND*);
