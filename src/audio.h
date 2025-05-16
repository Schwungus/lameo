#pragma once

#include <fmod.h>

#include "asset.h"

void audio_init();
void audio_update();
void audio_teardown();

FMOD_RESULT audio_debug_callback(FMOD_DEBUG_FLAGS, const char*, int, const char*, const char*);

inline void load_sample(const char*, FMOD_SOUND**);
inline void destroy_sample(FMOD_SOUND*);

FMOD_CHANNEL* play_ui_sound(const struct Sound*, bool, uint32_t, float, float);

inline void load_stream(const char*, FMOD_SOUND**);
inline void destroy_stream(FMOD_SOUND*);
