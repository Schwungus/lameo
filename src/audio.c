#include <SDL3/SDL_stdinc.h>
#include <fmod_errors.h>

#include "asset.h"
#include "audio.h"
#include "log.h"

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

    result = FMOD_System_Init(speaker, MAX_CHANNELS, FMOD_INIT_STREAM_FROM_UPDATE | FMOD_INIT_MIX_FROM_UPDATE, NULL);
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

static int dummy = 0, dummy2 = 0, dummy3 = 0;
static TrackID dummy4 = 0;
void audio_update() {
    if (!dummy) {
        const TrackID hid = fetch_track_hid("sample");
        const struct Track* track = hid_to_track(hid);
        if (track != NULL)
            FMOD_System_PlaySound(speaker, track->stream, music_group, false, NULL);
        dummy = 1;
    }

    if (!dummy2) {
        dummy4 = fetch_track_hid("sample2");
        dummy2 = 1;
    } else if (++dummy3 >= 100) {
        const struct Track* track = hid_to_track(dummy4);
        if (track != NULL)
            FMOD_System_PlaySound(speaker, track->stream, music_group, false, NULL);
        dummy3 = 0;
    }

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

void load_stream(const char* filename, FMOD_SOUND** stream) {
    FMOD_RESULT result =
        FMOD_System_CreateSound(speaker, filename, FMOD_CREATESTREAM | FMOD_ACCURATETIME, NULL, stream);
    if (result != FMOD_OK)
        FATAL("Stream fail: %s", FMOD_ErrorString(result));
}

void destroy_stream(FMOD_SOUND* stream) {
    FMOD_Sound_Release(stream);
}
