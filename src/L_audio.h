#pragma once

#include <fmod.h>

#include "L_actor.h"
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

struct Actor; // Fixes a warning in L_script.h

void audio_init();
void audio_update();
void audio_teardown();

void load_sample(const char*, FMOD_SOUND**);
void destroy_sample(FMOD_SOUND*);

void set_listeners(int);
void update_listener(int, const float*, const float*, const float*, const float*);

void pause_world_sounds(bool);
FMOD_CHANNELGROUP* create_world_sound_pool();
void destroy_world_sound_pool(FMOD_CHANNELGROUP*);
FMOD_CHANNELGROUP* create_emitter(struct Actor*);
void destroy_emitter(FMOD_CHANNELGROUP*);

FMOD_CHANNEL* play_ui_sound(struct Sound*, bool, uint32_t, float, float);
FMOD_CHANNEL* play_actor_sound(struct Actor*, struct Sound*, float, float, bool, uint32_t, float, float);

void load_stream(const char*, FMOD_SOUND**);
void destroy_stream(FMOD_SOUND*);
