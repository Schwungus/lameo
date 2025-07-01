#include "L_player.h"
#include "L_log.h"
#include "L_memory.h"

static FlagsID default_pflags = 0;

static struct Player players[MAX_PLAYERS] = {0};
static struct Player* ready_players = NULL;
static struct Player* active_players = NULL;

void player_init() {
    if ((default_pflags = (FlagsID)SDL_CreateProperties()) == 0)
        FATAL("Player init fail: %s", SDL_GetError());

    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].slot = i;

        players[i].status = PS_INACTIVE;
        players[i].previous_ready = players[i].next_ready = NULL;
        players[i].previous_active = players[i].next_active = NULL;
        players[i].previous_neighbor = players[i].next_neighbor = NULL;

        players[i].input.move[0] = players[i].input.move[1] = 0;
        players[i].input.aim[0] = players[i].input.aim[1] = 0;
        players[i].input.buttons = PB_NONE;
        players[i].last_input = players[i].input;

        if ((players[i].flags = (FlagsID)SDL_CreateProperties()) == 0)
            FATAL("Player %d flags fail: %s", i, SDL_GetError());

        players[i].room = players[i].actor = players[i].camera = NULL;
    }

    activate_player(0);

    INFO("Opened");
}

void player_teardown() {
    for (int i = 0; i < MAX_PLAYERS; i++)
        CLOSE_HANDLE(players[i].flags, SDL_DestroyProperties);
    ready_players = active_players = NULL;

    CLOSE_HANDLE(default_pflags, SDL_DestroyProperties);

    INFO("Closed");
}

int activate_player(int slot) {
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

int deactivate_player(int slot) {
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

// Player iterators
struct Player* get_player(int slot) {
    return IS_VALID_PSLOT(slot) ? &players[slot] : NULL;
}

int next_ready_player(int slot) {
    if (IS_INVALID_PSLOT(slot))
        return ready_players == NULL ? -1 : ready_players->slot;
    return players[slot].next_ready == NULL ? -1 : players[slot].next_ready->slot;
}

int next_active_player(int slot) {
    if (IS_INVALID_PSLOT(slot))
        return active_players == NULL ? -1 : active_players->slot;
    return players[slot].next_active == NULL ? -1 : players[slot].next_active->slot;
}

int next_neighbor_player(int slot) {
    return (IS_INVALID_PSLOT(slot) || players[slot].next_neighbor == NULL) ? -1 : players[slot].next_neighbor->slot;
}

// Flag getters
enum FlagTypes get_pflag_type(int slot, const char* name) {
    return IS_INVALID_PSLOT(slot) ? FT_NULL
                                  : (enum FlagTypes)SDL_GetPropertyType((SDL_PropertiesID)players[slot].flags, name);
}

bool get_bool_pflag(int slot, const char* name, bool default_value) {
    return IS_INVALID_PSLOT(slot) ? default_value
                                  : SDL_GetBooleanProperty((SDL_PropertiesID)players[slot].flags, name, default_value);
}

Sint64 get_int_pflag(int slot, const char* name, Sint64 default_value) {
    return IS_INVALID_PSLOT(slot) ? default_value
                                  : SDL_GetNumberProperty((SDL_PropertiesID)players[slot].flags, name, default_value);
}

float get_float_pflag(int slot, const char* name, float default_value) {
    return IS_INVALID_PSLOT(slot) ? default_value
                                  : SDL_GetFloatProperty((SDL_PropertiesID)players[slot].flags, name, default_value);
}

const char* get_string_pflag(int slot, const char* name, const char* default_value) {
    return IS_INVALID_PSLOT(slot) ? default_value
                                  : SDL_GetStringProperty((SDL_PropertiesID)players[slot].flags, name, default_value);
}

// Flag setters
void set_bool_pflag(int slot, const char* name, bool value) {
    if (IS_VALID_PSLOT(slot))
        SDL_SetBooleanProperty((SDL_PropertiesID)players[slot].flags, name, value);
}

void set_int_pflag(int slot, const char* name, Sint64 value) {
    if (IS_VALID_PSLOT(slot))
        SDL_SetNumberProperty((SDL_PropertiesID)players[slot].flags, name, value);
}

void set_float_pflag(int slot, const char* name, float value) {
    if (IS_VALID_PSLOT(slot))
        SDL_SetFloatProperty((SDL_PropertiesID)players[slot].flags, name, value);
}

void set_string_pflag(int slot, const char* name, const char* value) {
    if (IS_VALID_PSLOT(slot))
        SDL_SetStringProperty((SDL_PropertiesID)players[slot].flags, name, value);
}

// Flag utilities
void reset_pflag(int slot, const char* name) {
    if (IS_VALID_PSLOT(slot))
        SDL_ClearProperty((SDL_PropertiesID)players[slot].flags, name);
}

bool toggle_pflag(int slot, const char* name) {
    if (IS_INVALID_PSLOT(slot))
        return false;
    bool b = !(SDL_GetBooleanProperty((SDL_PropertiesID)players[slot].flags, name, false));
    SDL_SetBooleanProperty((SDL_PropertiesID)players[slot].flags, name, b);
    return b;
}

Sint64 increment_pflag(int slot, const char* name) {
    if (IS_INVALID_PSLOT(slot))
        return 0;
    Sint64 i = SDL_GetNumberProperty((SDL_PropertiesID)players[slot].flags, name, 0) + 1;
    SDL_SetNumberProperty((SDL_PropertiesID)players[slot].flags, name, i);
    return i;
}

Sint64 decrement_pflag(int slot, const char* name) {
    if (IS_INVALID_PSLOT(slot))
        return 0;
    Sint64 i = SDL_GetNumberProperty((SDL_PropertiesID)players[slot].flags, name, 0) - 1;
    SDL_SetNumberProperty((SDL_PropertiesID)players[slot].flags, name, i);
    return i;
}
