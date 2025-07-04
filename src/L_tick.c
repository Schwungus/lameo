#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include "L_input.h"
#include "L_internal.h"
#include "L_log.h"
#include "L_tick.h"
#include "L_ui.h"

static uint64_t last_time = 0;
static float ticks = 0;

void tick_init() {
    last_time = SDL_GetTicks();

    INFO("Opened");
}

void tick_update() {
    const uint64_t current_time = SDL_GetTicks();
    ticks += (float)(current_time - last_time) * ((float)TICKRATE / 1000.);

    if (ticks >= 1) {
        if (get_load_state() == LOAD_NONE)
            while (ticks >= 1) {
                bool tick_world = true;

                // UI
                struct UI* ui_top = get_ui_top();
                if (ui_top == NULL && input_pressed(VERB_PAUSE, 0)) {
                    create_ui(NULL, "Pause");
                    input_consume(VERB_UI_UP);
                    input_consume(VERB_UI_LEFT);
                    input_consume(VERB_UI_DOWN);
                    input_consume(VERB_UI_RIGHT);
                    input_consume(VERB_UI_ENTER);
                    input_consume(VERB_UI_CLICK);
                    input_consume(VERB_UI_BACK);
                    ui_top = get_ui_top();
                }
                if (ui_top != NULL) {
                    update_ui_input();
                    if (ui_top->type->tick != LUA_NOREF)
                        execute_ref_in(ui_top->type->tick, ui_top->hid, ui_top->type->name);
                    if ((ui_top = get_ui_top()) == NULL) {
                        input_consume(VERB_PAUSE);
                    } else {
                        if (ui_top->flags & UIF_BLOCKING)
                            tick_world = false;
                    }
                }

                // World
                if (tick_world) {}

                input_clear_momentary();
                ticks -= 1;
            }
        else
            ticks -= SDL_floorf(ticks);
    }

    last_time = current_time;
}

void tick_teardown() {
    INFO("Closed");
}
