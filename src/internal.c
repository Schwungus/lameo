#include <SDL3/SDL.h>

#include "asset.h"
#include "audio.h"
#include "config.h"
#include "flags.h"
#include "input.h"
#include "internal.h"
#include "log.h"
#include "mod.h"
#include "player.h"
#include "tick.h"
#include "video.h"

void init(const char* config_path) {
    log_init();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        FATAL("SDL fail: %s", SDL_GetError());
    video_init();
    audio_init();
    input_init();
    config_init(config_path);
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

                case SDL_EVENT_KEYBOARD_ADDED:
                case SDL_EVENT_KEYBOARD_REMOVED:
                    handle_keyboard((SDL_KeyboardDeviceEvent*)&event);
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    handle_key((SDL_KeyboardEvent*)&event);
                    break;

                case SDL_EVENT_MOUSE_ADDED:
                case SDL_EVENT_MOUSE_REMOVED:
                    handle_mouse((SDL_MouseDeviceEvent*)&event);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    handle_mouse_button((SDL_MouseButtonEvent*)&event);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    handle_mouse_wheel((SDL_MouseWheelEvent*)&event);
                    break;
                    /*case SDL_EVENT_MOUSE_MOTION:
                        handle_mouse_motion(&event);
                        break;

                    case SDL_EVENT_GAMEPAD_ADDED:
                    case SDL_EVENT_GAMEPAD_REMOVED:
                        handle_gamepad(&event);
                        break;
                    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    case SDL_EVENT_GAMEPAD_BUTTON_UP:
                        handle_gamepad_button(&event);
                        break;
                    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                        handle_gamepad_axis(&event);
                        break;
                    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
                        handle_gamepad_sensor(&event);
                        break;
                    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
                    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
                    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
                        handle_gamepad_touchpad(&event);
                        break;*/
            }
        if (!running)
            break;

        tick_update();
        video_update();
        audio_update();
        input_update();
    }
}

void cleanup() {
    tick_teardown();
    script_teardown();
    asset_teardown();
    mod_teardown();
    player_teardown();
    flags_teardown();
    config_teardown();
    input_teardown();
    audio_teardown();
    video_teardown();
    log_teardown();
    SDL_Quit();
}
