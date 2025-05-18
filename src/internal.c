#include <SDL3/SDL.h>

#include "asset.h"
#include "audio.h"
#include "flags.h"
#include "internal.h"
#include "log.h"
#include "mod.h"
#include "player.h"
#include "video.h"

#define TICKRATE 30

void init() {
    log_init();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        FATAL("SDL fail: %s", SDL_GetError());
    video_init();
    audio_init();
    flags_init();
    player_init();
    mod_init();
    asset_init();
}

void loop() {
    bool running = true;
    uint64_t last_time = SDL_GetTicks();
    float ticks = 0;
    while (running) {
        // Events
        SDL_Event event;
        while (SDL_PollEvent(&event))
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_DOWN: {
                    play_ui_sound(hid_to_sound(fetch_sound_hid("tick")), false, 0, 1, 1);
                    break;
                }
            }
        if (!running)
            break;

        // Tick
        const uint64_t current_time = SDL_GetTicks();
        ticks += (float)(current_time - last_time) * ((float)TICKRATE / 1000.);
        while (ticks >= 1) {
            // UI, game ...
            ticks -= 1;
        }
        last_time = current_time;

        // Draw
        video_update();
        audio_update();
    }
}

void cleanup() {
    asset_teardown();
    mod_teardown();
    player_teardown();
    flags_teardown();
    audio_teardown();
    video_teardown();
    log_teardown();
    SDL_Quit();
}
