#include <SDL3/SDL.h>

#include "L_actor.h"
#include "L_asset.h"
#include "L_audio.h"
#include "L_config.h"
#include "L_file.h"
#include "L_flags.h"
#include "L_handler.h"
#include "L_input.h"
#include "L_internal.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_mod.h"
#include "L_player.h"
#include "L_steam.h"
#include "L_tick.h"
#include "L_ui.h"
#include "L_video.h"

static struct LoadState load_state = {0};

void init(const char* config_path, const char* controls_path) {
    log_init();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        FATAL("SDL fail: %s", SDL_GetError());
    steam_init();
    file_init();
    video_init();
    audio_init();
    input_init();
    config_init(config_path, controls_path);
    flags_init();
    player_init();
    mod_init();
    localize_init();
    asset_init();

    // Initialize actors and UIs before scripting
    actor_init();
    ui_init();
    script_init();

    handler_init();
    tick_init();

    // Default level is "main"
    SDL_strlcpy(load_state.level, "main", LEVEL_NAME_MAX);
    load_state.state = LOAD_START;
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
                    handle_keyboard(&event.kdevice);
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    handle_key(&event.key);
                    break;

                case SDL_EVENT_MOUSE_ADDED:
                case SDL_EVENT_MOUSE_REMOVED:
                    handle_mouse(&event.mdevice);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    handle_mouse_button(&event.button);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    handle_mouse_wheel(&event.wheel);
                    break;

                case SDL_EVENT_GAMEPAD_ADDED:
                case SDL_EVENT_GAMEPAD_REMOVED:
                    handle_gamepad(&event.gdevice);
                    break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                    handle_gamepad_button(&event.gbutton);
                    break;
                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                    handle_gamepad_axis(&event.gaxis);
                    break;
            }
        if (!running)
            break;

        switch (load_state.state) {
            default:
                break;

            case LOAD_START: {
                load_state.state = LOAD_UNLOAD;
                break;
            }

            case LOAD_UNLOAD: {
                // Unload level
                struct UI* ui_root = get_ui_root();
                if (ui_root != NULL)
                    destroy_ui(ui_root);

                // Unload assets
                clear_assets(0);

                load_state.state = LOAD_LOAD;
                break;
            }

            case LOAD_LOAD: {
                INFO("\n\nENTERING \"%s\" (room %d, tag %d)\n", load_state.level, load_state.room, load_state.tag);

                // Load level
                char level_file_helper[FILE_PATH_MAX];
                SDL_snprintf(level_file_helper, sizeof(level_file_helper), "levels/%s.json", load_state.level);
                yyjson_doc* json = load_json(get_mod_file(level_file_helper, NULL));
                if (json == NULL)
                    FATAL("Invalid level \"%s\"", load_state.level);
                yyjson_doc_free(json);

                load_state.state = LOAD_END;
                break;
            }

            case LOAD_END: {
                // Ready players to active
                // Assign players to room ID "load_state.room"
                create_ui(NULL, "Pause");

                load_state.state = LOAD_NONE;
                break;
            }
        }

        steam_update();
        tick_update();
        video_update();
        audio_update();
        input_update();

        if (load_state.state != LOAD_NONE && load_state.level[0] == '\0')
            running = false;
    }
}

void cleanup() {
    tick_teardown();
    handler_teardown();

    // Free actors and UIs before scripting
    ui_teardown();
    actor_teardown();
    script_teardown();

    asset_teardown();
    localize_teardown();
    mod_teardown();
    player_teardown();
    flags_teardown();
    config_teardown();
    input_teardown();
    audio_teardown();
    video_teardown();
    file_teardown();
    steam_teardown();
    log_teardown();
    SDL_Quit();
}

enum LoadStates get_load_state() {
    return load_state.state;
}
