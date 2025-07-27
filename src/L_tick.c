#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include "L_config.h"
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
    ticks += (float)(current_time - last_time) * ((float)TICKRATE / 1000.0f);

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
                        execute_ref_in(ui_top->type->tick, ui_top->userdata, ui_top->type->name);
                    ui_top = get_ui_top();
                    if (ui_top == NULL) {
                        input_consume(VERB_PAUSE);
                    } else {
                        if (ui_top->flags & UIF_BLOCKING)
                            tick_world = false;
                    }
                } else {
                    if (input_pressed(VERB_ATTACK, 0)) {
                        struct Player* player = get_active_players();
                        if (player != NULL && player->room != NULL) {
                            struct Actor* actor = player->room->actors;
                            if (actor != NULL) {
                                int i = SDL_rand(10);
                                while (i-- && actor->previous_neighbor != NULL)
                                    actor = actor->previous_neighbor;
                                destroy_actor_later(actor, true);
                            }
                        }
                    }

                    if (input_pressed(VERB_AIM, 0)) {
                        struct Player* player = get_active_players();
                        if (player != NULL)
                            player_enter_room(player, (player->room == NULL || player->room->id != 0) ? 0 : 25);
                    }

                    if (input_pressed(VERB_LEAVE, 0)) {
                        struct Player* player = get_active_players();
                        if (player != NULL)
                            player_leave_room(player);
                    }

                    if (input_pressed(VERB_INTERACT, 0)) {
                        go_to_level("main", 25, 0);
                        tick_world = false;
                    }
                }

                // World
                if (tick_world) {
                    struct Player* player = get_active_players();
                    while (player != NULL) {
                        player->last_input = player->input;

                        const float walk = (input_value(VERB_WALK, player->slot) == 0) ? 1 : 0.5f;
                        player->input.move[0] = (int16_t)((float)(input_value(VERB_DOWN, player->slot) -
                                                                  input_value(VERB_UP, player->slot)) *
                                                          walk);
                        player->input.move[1] = (int16_t)((float)(input_value(VERB_RIGHT, player->slot) -
                                                                  input_value(VERB_LEFT, player->slot)) *
                                                          walk);

                        player->input.aim[0] =
                            (int16_t)((float)(input_value(VERB_AIM_RIGHT, player->slot) -
                                              input_value(VERB_AIM_LEFT, player->slot)) *
                                      get_float_cvar("in_aim_x") * (float)(get_bool_cvar("in_invert_x") ? -1 : 1));
                        player->input.aim[1] =
                            (int16_t)((float)(input_value(VERB_AIM_DOWN, player->slot) -
                                              input_value(VERB_AIM_UP, player->slot)) *
                                      get_float_cvar("in_aim_y") * (float)(get_bool_cvar("in_invert_y") ? -1 : 1));

                        player->input.buttons = PB_NONE;
                        if (input_value(VERB_JUMP, player->slot))
                            player->input.buttons |= PB_JUMP;
                        if (input_value(VERB_INTERACT, player->slot))
                            player->input.buttons |= PB_INTERACT;
                        if (input_value(VERB_ATTACK, player->slot))
                            player->input.buttons |= PB_ATTACK;
                        if (input_value(VERB_INVENTORY1, player->slot))
                            player->input.buttons |= PB_INVENTORY1;
                        if (input_value(VERB_INVENTORY2, player->slot))
                            player->input.buttons |= PB_INVENTORY2;
                        if (input_value(VERB_INVENTORY3, player->slot))
                            player->input.buttons |= PB_INVENTORY3;
                        if (input_value(VERB_INVENTORY4, player->slot))
                            player->input.buttons |= PB_INVENTORY4;
                        if (input_value(VERB_AIM, player->slot))
                            player->input.buttons |= PB_AIM;

                        player = player->previous_active;
                    }

                    player = get_active_players();
                    while (player != NULL) {
                        if (player->room != NULL && player->room->master == player) {
                            struct Actor* actor = player->room->actors;
                            while (actor != NULL) {
                                // TODO: Culling
                                struct Actor* next = actor->previous_neighbor;
                                if (!(actor->flags & (AF_CULLED | AF_FROZEN | AF_DESTROYED)))
                                    tick_actor(actor);
                                if (actor->flags & AF_DESTROYED)
                                    destroy_actor(actor, actor->flags & AF_DESTROYED_NATURAL, true);
                                actor = next;
                            }
                        }
                        player = player->previous_active;
                    }
                }

                input_clear_momentary();
                if (get_load_state() != LOAD_NONE) {
                    ticks -= SDL_floorf(ticks);
                    break;
                }
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
