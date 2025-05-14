#pragma once

#include <fmod.h>

void audio_init();
void audio_update();
void audio_teardown();

FMOD_RESULT audio_debug_callback(FMOD_DEBUG_FLAGS, const char*, int, const char*, const char*);

void load_sample(const char*, FMOD_SOUND**);
void destroy_sample(FMOD_SOUND*);

void load_stream(const char*, FMOD_SOUND**);
void destroy_stream(FMOD_SOUND*);
