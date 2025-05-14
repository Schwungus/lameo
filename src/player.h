#pragma once

#include <SDL3/SDL_properties.h>

enum PlayerStatus {
    PS_INACTIVE,
    PS_READY,
    PS_ACTIVE,
};

enum PlayerButtons {
    PB_NONE = 0b00000000,

    PB_JUMP = 0b00000001,
    PB_INTERACT = 0b00000010,
    PB_ATTACK = 0b00000100,

    PB_INVENTORY1 = 0b00001000,
    PB_INVENTORY2 = 0b00010000,
    PB_INVENTORY3 = 0b00100000,
    PB_INVENTORY4 = 0b01000000,

    PB_AIM = 0b10000000,
};

struct PlayerInput {
    int8_t move_x, move_y;
    int16_t aim_x, aim_y;
    uint8_t buttons;
};

struct Player {
    uint8_t slot;

    enum PlayerStatus status;
    struct Player *previous_ready, *next_ready;
    struct Player *previous_active, *next_active;

    struct PlayerInput input, last_input;

    SDL_PropertiesID states;

    void *room, *actor, *camera;
};

void player_init();
int player_activate(int slot);
int player_deactivate(int slot);
void player_teardown();
