#pragma once

#include "L_flags.h"

#define MAX_PLAYERS 4

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
    int8_t move[2];
    int16_t aim[2];
    uint8_t buttons;
};

struct Player {
    uint8_t slot;

    enum PlayerStatus status;
    struct Player *previous_ready, *next_ready;
    struct Player *previous_active, *next_active;
    struct Player *previous_neighbor, *next_neighbor;

    struct PlayerInput input, last_input;

    FlagsID flags;

    void *room, *actor, *camera;
};

void player_init();
void player_teardown();

int activate_player(int);
int deactivate_player(int);

struct Player* get_player(int);

int next_ready_player(int);
int next_active_player(int);
int next_neighbor_player(int);

#define IS_INVALID_PSLOT(slot) (slot < 0 || slot >= MAX_PLAYERS)
#define IS_VALID_PSLOT(slot) (slot >= 0 && slot < MAX_PLAYERS)

enum FlagTypes get_pflag_type(int, const char*);
bool get_bool_pflag(int, const char*, bool);
Sint64 get_int_pflag(int, const char*, Sint64);
float get_float_pflag(int, const char*, float);
const char* get_string_pflag(int, const char*, const char*);

void set_bool_pflag(int, const char*, bool);
void set_int_pflag(int, const char*, Sint64);
void set_float_pflag(int, const char*, float);
void set_string_pflag(int, const char*, const char*);

void reset_pflag(int, const char*);
bool toggle_pflag(int, const char*);
Sint64 increment_pflag(int, const char*);
Sint64 decrement_pflag(int, const char*);
