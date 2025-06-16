#include <SDL3/SDL.h>

#include "asset.h"
#include "audio.h"
#include "flags.h"
#include "internal.h"
#include "log.h"
#include "mod.h"
#include "player.h"
#include "tick.h"
#include "video.h"

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
    script_init();
    tick_init();
}

void loop() {
    bool running = true;
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

        tick_update();
        video_update();
        audio_update();
    }
}

void cleanup() {
    tick_teardown();
    script_teardown();
    asset_teardown();
    mod_teardown();
    player_teardown();
    flags_teardown();
    audio_teardown();
    video_teardown();
    log_teardown();
    SDL_Quit();
}
