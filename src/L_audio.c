#include <fmod_errors.h>

#include "L_audio.h"
#include "L_log.h"
#include "L_math.h"
#include "L_memory.h"

#define MAX_CHANNELS 256

static FMOD_SYSTEM* speaker = NULL;
static FMOD_CHANNELGROUP *sound_group = NULL, *world_group = NULL, *ui_group = NULL;
static FMOD_CHANNELGROUP* music_group = NULL;

void audio_init() {
    // Debug
    FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_WARNING, FMOD_DEBUG_MODE_CALLBACK, audio_debug_callback, NULL);

    // System
    FMOD_RESULT result = FMOD_System_Create(&speaker, FMOD_VERSION);
    if (result != FMOD_OK)
        FATAL("Audio create fail: %s", FMOD_ErrorString(result));

    result = FMOD_System_Init(speaker, MAX_CHANNELS, FMOD_INIT_NORMAL, NULL);
    if (result != FMOD_OK)
        FATAL("Audio init fail: %s", FMOD_ErrorString(result));

    // Version
    uint32_t version, buildnumber;
    FMOD_System_GetVersion(speaker, &version, &buildnumber);
    INFO(
        "FMOD version: %u.%02u.%02u - Build %u", (version >> 16) & 0xFFFF, (version >> 8) & 0xFF, version & 0xFF,
        buildnumber
    );

    // Groups
    FMOD_System_CreateChannelGroup(speaker, "sound", &sound_group);
    FMOD_System_CreateChannelGroup(speaker, "world", &world_group);
    FMOD_System_CreateChannelGroup(speaker, "ui", &ui_group);
    FMOD_ChannelGroup_AddGroup(sound_group, world_group, 1, NULL);
    FMOD_ChannelGroup_AddGroup(sound_group, ui_group, 1, NULL);

    FMOD_System_CreateChannelGroup(speaker, "music", &music_group);

    INFO("Opened");
}

void audio_update() {
    FMOD_System_Update(speaker);
}

void audio_teardown() {
    CLOSE_POINTER(ui_group, FMOD_ChannelGroup_Release);
    CLOSE_POINTER(world_group, FMOD_ChannelGroup_Release);
    CLOSE_POINTER(sound_group, FMOD_ChannelGroup_Release);
    CLOSE_POINTER(music_group, FMOD_ChannelGroup_Release);

    CLOSE_POINTER(speaker, FMOD_System_Release);

    INFO("Closed");
}

FMOD_RESULT
audio_debug_callback(FMOD_DEBUG_FLAGS flags, const char* file, int line, const char* func, const char* message) {
    log_generic(src_basename(file), line, "%s: %s", func, message);
    return FMOD_OK;
}

// Sound
void load_sample(const char* filename, FMOD_SOUND** sample) {
    FMOD_RESULT result = FMOD_System_CreateSound(speaker, filename, FMOD_CREATESAMPLE, NULL, sample);
    if (result != FMOD_OK) {
        WTF("Sample fail: %s", FMOD_ErrorString(result));
        *sample = NULL;
    }
}

void destroy_sample(FMOD_SOUND* sample) {
    FMOD_Sound_Release(sample);
}

FMOD_CHANNEL* play_ui_sound(struct Sound* sound, bool loop, uint32_t offset, float pitch, float gain) {
    if (sound == NULL || sound->samples == NULL)
        return NULL;

    FMOD_SOUND* sample = sound->samples[SDL_rand(sound->num_samples)];
    if (sample == NULL)
        return NULL;

    FMOD_CHANNEL* instance;
    FMOD_System_PlaySound(speaker, sample, ui_group, true, &instance);
    FMOD_Channel_SetMode(instance, loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    FMOD_Channel_SetPosition(instance, offset, FMOD_TIMEUNIT_MS);
    FMOD_Channel_SetPitch(instance, pitch * lerp(sound->pitch[0], sound->pitch[1], SDL_randf()));
    FMOD_Channel_SetVolume(instance, gain * sound->gain);
    FMOD_Channel_SetPaused(instance, false);
    return instance;
}

// Music
void load_stream(const char* filename, FMOD_SOUND** stream) {
    FMOD_RESULT result =
        FMOD_System_CreateSound(speaker, filename, FMOD_CREATESTREAM | FMOD_ACCURATETIME, NULL, stream);
    if (result != FMOD_OK) {
        WTF("Stream fail: %s", FMOD_ErrorString(result));
        *stream = NULL;
    }
}

void destroy_stream(FMOD_SOUND* stream) {
    FMOD_Sound_Release(stream);
}
