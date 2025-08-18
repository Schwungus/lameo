#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include "L_config.h"
#include "L_input.h"
#include "L_internal.h"
#include "L_log.h"
#include "L_tick.h"
#include "L_ui.h"
#include "L_video.h"

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
        if (get_load_state() == LOAD_NONE) {
            // Pre-interpolation
            struct Actor* actor = get_actors();
            while (actor != NULL) {
                glm_vec3_copy(actor->pos, actor->draw_pos[0]);
                glm_vec3_copy(actor->angle, actor->draw_angle[0]);

                struct ActorCamera* camera = actor->camera;
                if (camera != NULL) {
                    glm_vec3_copy(camera->pos, camera->draw_pos[0]);
                    glm_vec3_copy(camera->angle, camera->draw_angle[0]);
                    camera->draw_fov[0] = camera->fov;
                    camera->draw_range[0] = camera->range;
                }

                struct ActorLight* light = actor->light;
                if (light != NULL)
                    lame_copy(light->draw_args[0], light->draw_args[1], RL_ARGS * sizeof(GLfloat));

                struct ModelInstance* model = actor->model;
                if (model != NULL) {
                    glm_vec3_copy(model->pos, model->draw_pos[0]);
                    glm_vec3_copy(model->angle, model->draw_angle[0]);
                    glm_vec3_copy(model->scale, model->draw_scale[0]);
                    if (model->animation != NULL)
                        lame_copy(
                            model->draw_sample[0], model->sample, model->model->num_nodes * sizeof(DualQuaternion)
                        );
                }

                actor = actor->previous;
            }

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

                        float dx = (int16_t)((float)(input_value(VERB_AIM_RIGHT, player->slot) -
                                                     input_value(VERB_AIM_LEFT, player->slot)) *
                                             get_float_cvar("in_aim_x"));
                        float dy = (int16_t)((float)(input_value(VERB_AIM_DOWN, player->slot) -
                                                     input_value(VERB_AIM_UP, player->slot)) *
                                             get_float_cvar("in_aim_y"));
                        if (player->slot == 0) {
                            const vec2* mouse_delta = get_mouse_delta();
                            dx += (*mouse_delta)[0] * get_float_cvar("in_mouse_x");
                            dy += (*mouse_delta)[1] * get_float_cvar("in_mouse_y");
                        }
                        player->input.aim[0] = (int16_t)(dx * (float)(get_bool_cvar("in_invert_x") ? -1 : 1));
                        player->input.aim[1] = (int16_t)(dy * (float)(get_bool_cvar("in_invert_y") ? -1 : 1));

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
        } else {
            ticks -= SDL_floorf(ticks);
        }
    }

    // Post-interpolation
    struct Actor* actor = get_actors();
    if (get_framerate() <= TICKRATE) {
        while (actor != NULL) {
            glm_vec3_copy(actor->pos, actor->draw_pos[1]);
            glm_vec3_copy(actor->angle, actor->draw_angle[1]);

            struct ActorCamera* camera = actor->camera;
            if (camera != NULL) {
                glm_vec3_copy(camera->pos, camera->draw_pos[1]);
                glm_vec3_copy(camera->angle, camera->draw_angle[1]);
                camera->draw_fov[1] = camera->fov;
                camera->draw_range[1] = camera->range;
            }

            struct ActorLight* light = actor->light;
            if (light != NULL) {
                struct RoomLight* rlight = light->light;
                glm_vec3_copy(actor->pos, rlight->pos);
                lame_copy(rlight->args, light->draw_args[1], RL_ARGS * sizeof(GLfloat));
            }

            struct ModelInstance* model = actor->model;
            if (model != NULL) {
                glm_vec3_copy(model->pos, model->draw_pos[1]);
                glm_vec3_copy(model->angle, model->draw_angle[1]);
                glm_vec3_copy(model->scale, model->draw_scale[1]);
                if (model->animation != NULL)
                    lame_copy(model->draw_sample[1], model->sample, model->model->num_nodes * sizeof(DualQuaternion));
            }

            actor = actor->previous;
        }
    } else {
        while (actor != NULL) {
            glm_vec3_lerp(actor->draw_pos[0], actor->pos, ticks, actor->draw_pos[1]);
            actor->draw_angle[1][0] = glm_lerp(actor->draw_angle[0][0], actor->angle[0], ticks);
            actor->draw_angle[1][1] = glm_lerp(actor->draw_angle[0][1], actor->angle[1], ticks);
            actor->draw_angle[1][2] = glm_lerp(actor->draw_angle[0][2], actor->angle[2], ticks);

            struct ActorCamera* camera = actor->camera;
            if (camera != NULL) {
                glm_vec3_lerp(camera->draw_pos[0], camera->pos, ticks, camera->draw_pos[1]);
                camera->draw_angle[1][0] = glm_lerp(camera->draw_angle[0][0], camera->angle[0], ticks);
                camera->draw_angle[1][1] = glm_lerp(camera->draw_angle[0][1], camera->angle[1], ticks);
                camera->draw_angle[1][2] = glm_lerp(camera->draw_angle[0][2], camera->angle[2], ticks);
                camera->draw_fov[1] = glm_lerp(camera->draw_fov[0], camera->fov, ticks);
                camera->draw_range[1] = glm_lerp(camera->draw_range[0], camera->range, ticks);
            }

            struct ActorLight* light = actor->light;
            if (light != NULL) {
                struct RoomLight* rlight = light->light;
                glm_vec3_copy(actor->draw_pos[1], rlight->pos);
                for (size_t i = 0; i < RL_ARGS; i++)
                    light->light->args[i] = glm_lerp(light->draw_args[0][i], light->draw_args[1][i], ticks);
            }

            struct ModelInstance* model = actor->model;
            if (model != NULL) {
                glm_vec3_lerp(model->draw_pos[0], model->pos, ticks, model->draw_pos[1]);
                model->draw_angle[1][0] = glm_lerp(model->draw_angle[0][0], model->angle[0], ticks);
                model->draw_angle[1][1] = glm_lerp(model->draw_angle[0][1], model->angle[1], ticks);
                model->draw_angle[1][2] = glm_lerp(model->draw_angle[0][2], model->angle[2], ticks);
                glm_vec3_lerp(model->draw_scale[0], model->scale, ticks, model->draw_scale[1]);
                if (model->animation != NULL)
                    for (size_t i = 0; i < model->model->num_nodes; i++)
                        dq_lerp(model->draw_sample[0][i], model->sample[i], ticks, model->draw_sample[1][i]);
            }

            actor = actor->previous;
        }
    }

    last_time = current_time;
}

void tick_teardown() {
    INFO("Closed");
}

void reset_ticks() {
    last_time = SDL_GetTicks();
    ticks = 0;
}

float get_ticks() {
    return ticks;
}
