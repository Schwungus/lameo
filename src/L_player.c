#include "L_player.h"
#include "L_level.h"
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

        players[i].room = NULL;
        players[i].actor = NULL;
    }

    activate_player(0);

    INFO("Opened");
}

void player_teardown() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].room != NULL)
            player_leave_room(&players[i]);
        if (players[i].actor != NULL)
            destroy_actor(players[i].actor, false);
        CLOSE_HANDLE(players[i].flags, SDL_DestroyProperties);
    }
    ready_players = active_players = NULL;

    CLOSE_HANDLE(default_pflags, SDL_DestroyProperties);

    INFO("Closed");
}

int activate_player(int slot) {
    if (IS_INVALID_PSLOT(slot))
        return -1;

    struct Player* player = &players[slot];
    if (player->status == PS_INACTIVE) {
        player->status = PS_READY;

        if (ready_players != NULL)
            ready_players->next_ready = player;
        player->previous_ready = ready_players;
        player->next_ready = NULL;
        ready_players = player;

        INFO("Player %d activated", slot);
        return 1;
    }

    WARN("Player %d already ready/active", slot);
    return 0;
}

int deactivate_player(int slot) {
    if (IS_INVALID_PSLOT(slot))
        return -1;
    if (slot == 0) {
        WARN("Player 0 is always active");
        return -1;
    }

    struct Player* player = &players[slot];
    if (player->status != PS_INACTIVE) {
        player_leave_room(player);
        player->status = PS_INACTIVE;

        if (player->previous_ready != NULL)
            player->previous_ready->next_ready = player->next_ready;
        if (player->next_ready != NULL)
            player->next_ready->previous_ready = player->previous_ready;
        if (ready_players == player)
            ready_players = player->previous_ready;
        player->previous_ready = player->next_ready = NULL;

        if (player->previous_active != NULL)
            player->previous_active->next_active = player->next_active;
        if (player->next_active != NULL)
            player->next_active->previous_active = player->previous_active;
        if (active_players == player)
            active_players = player->previous_active;
        player->previous_active = player->next_active = NULL;

        INFO("Player %d deactivated", slot);
        return 1;
    }

    WARN("Player %d already inactive", slot);
    return 0;
}

void dispatch_players() {
    struct Player* player = ready_players;
    while (player != NULL) {
        struct Player* it = player->previous_ready;
        player->status = PS_ACTIVE;
        player->previous_ready = player->next_ready = NULL;

        if (active_players != NULL)
            active_players->next_active = player;
        player->previous_active = active_players;
        player->next_active = NULL;
        active_players = player;

        INFO("Dispatched player %u", player->slot);
        player = it;
    }
    ready_players = NULL;
}

// Player iterators
struct Player* get_player(int slot) {
    return IS_VALID_PSLOT(slot) ? &players[slot] : NULL;
}

struct Player* get_ready_players() {
    return ready_players;
}

struct Player* get_active_players() {
    return active_players;
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

// Neighbors
struct Room* get_player_room(int slot) {
    return IS_VALID_PSLOT(slot) ? players[slot].room : NULL;
}

bool player_leave_room(struct Player* player) {
    struct Room* room = player->room;
    if (room == NULL)
        return false;

    if (player->actor != NULL)
        destroy_actor(player->actor, false);

    if (room->players == player)
        room->players = player->previous_neighbor;
    if (room->master == player)
        room->master = room->players;

    if (player->previous_neighbor != NULL)
        player->previous_neighbor->next_neighbor = player->next_neighbor;
    if (player->next_neighbor != NULL)
        player->next_neighbor->previous_neighbor = player->previous_neighbor;
    player->previous_neighbor = player->next_neighbor = NULL;
    player->room = NULL;

    if (room->master == NULL)
        deactivate_room(room);
    return true;
}

bool player_enter_room(struct Player* player, uint32_t id) {
    player_leave_room(player);

    struct Room* room = from_int_map(get_level()->rooms, id);
    if (room == NULL) {
        WTF("Player %u entering invalid room ID %u", player->slot, id);
        return false;
    }

    player->room = room;

    if (room->players != NULL)
        room->players->next_neighbor = player;
    player->previous_neighbor = room->players;
    player->next_neighbor = NULL;
    room->players = player;

    if (room->master == NULL) {
        room->master = player;
        activate_room(room);
    }
    return true;
}
