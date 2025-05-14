#include "player.h"
#include "log.h"
#include "mem.h"

#define MAX_PLAYERS 4

struct Player players[MAX_PLAYERS] = {0};
struct Player* ready_players = NULL;
struct Player* active_players = NULL;

void player_init() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].slot = i;

        players[i].status = PS_INACTIVE;
        players[i].previous_ready = NULL;
        players[i].next_ready = NULL;
        players[i].previous_active = NULL;
        players[i].next_active = NULL;

        players[i].input.move_x = 0;
        players[i].input.move_y = 0;
        players[i].input.aim_x = 0;
        players[i].input.aim_y = 0;
        players[i].input.buttons = PB_NONE;
        players[i].last_input = players[i].input;

        players[i].states = SDL_CreateProperties();
        if (players[i].states == 0)
            FATAL("Player %d state fail: %s", i, SDL_GetError());

        players[i].room = NULL;
        players[i].actor = NULL;
        players[i].camera = NULL;
    }

    player_activate(0);

    INFO("Opened");
}

int player_activate(int slot) {
    if (slot < 0 || slot >= MAX_PLAYERS)
        return -1;

    struct Player* player = &players[slot];
    if (player->status == PS_INACTIVE) {
        player->status = PS_READY;

        if (ready_players != NULL)
            ready_players->next_ready = player;
        player->previous_ready = ready_players;
        ready_players = player;

        INFO("Player %d activated", slot);
        return 1;
    }

    WARN("Player %d already ready/active", slot);
    return 0;
}

int player_deactivate(int slot) {
    if (slot < 0 || slot >= MAX_PLAYERS)
        return -1;

    if (slot == 0) {
        WARN("Player 0 is always active");
        return -1;
    }

    struct Player* player = &players[slot];
    if (player->status != PS_INACTIVE) {
        player->status = PS_INACTIVE;

        if (player->previous_ready != NULL)
            player->previous_ready->next_ready = player->next_ready;
        if (player->next_ready != NULL)
            player->next_ready->previous_ready = player->previous_ready;
        if (ready_players == player)
            ready_players = player->previous_ready;

        if (player->previous_active != NULL)
            player->previous_active->next_active = player->next_active;
        if (player->next_active != NULL)
            player->next_active->previous_active = player->previous_active;
        if (active_players == player)
            active_players = player->previous_active;

        INFO("Player %d deactivated", slot);
        return 1;
    }

    WARN("Player %d already inactive", slot);
    return 0;
}

void player_teardown() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        struct Player* player = &players[i];
        CLOSE_HANDLE(player->states, SDL_DestroyProperties);
    }

    ready_players = NULL;
    active_players = NULL;

    INFO("Closed");
}
