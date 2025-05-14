#include <SDL3/SDL.h>

#include "asset.h"
#include "audio.h"
#include "internal.h"
#include "log.h"
#include "mod.h"
#include "player.h"
#include "video.h"

#define TICKRATE 30
#define TICKTIME (1000 / TICKRATE)

void init() {
    log_init();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        FATAL("SDL fail: %s", SDL_GetError());
    video_init();
    audio_init();
    mod_init();
    asset_init();
    player_init();
}

void loop() {
    bool running = true;

    while (running) {
        const uint64_t start = SDL_GetTicks();

        // Events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        if (!running)
            break;

        // Tick

        // Draw
        video_update();
        audio_update();

        // Wait
        const uint64_t delta = start - SDL_GetTicks();
        if (delta < TICKTIME)
            SDL_Delay(TICKTIME - delta);
    }
}

void cleanup() {
    player_teardown();
    asset_teardown();
    mod_teardown();
    audio_teardown();
    video_teardown();
    SDL_Quit();
    log_teardown();
}
