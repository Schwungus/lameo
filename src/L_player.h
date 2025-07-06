#pragma once

#include "L_actor.h"
#include "L_room.h"

#define MAX_PLAYERS 4

#define IS_INVALID_PSLOT(slot) (slot < 0 || slot >= MAX_PLAYERS)
#define IS_VALID_PSLOT(slot) (slot >= 0 && slot < MAX_PLAYERS)

enum PlayerStatus {
    PS_INACTIVE,
    PS_READY,
    PS_ACTIVE,
};

enum PlayerButtons {
    PB_NONE = 0,

    PB_JUMP = 1 << 0,
    PB_INTERACT = 1 << 1,
    PB_ATTACK = 1 << 2,

    PB_INVENTORY1 = 1 << 3,
    PB_INVENTORY2 = 1 << 4,
    PB_INVENTORY3 = 1 << 5,
    PB_INVENTORY4 = 1 << 6,

    PB_AIM = 1 << 7,
};

struct PlayerInput {
    int8_t move[2];
    int16_t aim[2];
    enum PlayerButtons buttons;
};

struct Player {
    uint8_t slot;

    enum PlayerStatus status;
    struct Player *previous_ready, *next_ready;       // Position in list (previous-order)
    struct Player *previous_active, *next_active;     // Position in list (previous-order)
    struct Player *previous_neighbor, *next_neighbor; // Position in list (previous-order)

    struct PlayerInput input, last_input;

    struct Room* room;
    struct Actor* actor;
};

void player_init();
void player_teardown();

int activate_player(int);
int deactivate_player(int);
void dispatch_players();

struct Player* get_player(int);
struct Player* get_ready_players();
struct Player* get_active_players();

bool player_enter_room(struct Player*, uint32_t);
bool player_leave_room(struct Player*);
