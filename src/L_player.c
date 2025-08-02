#include "L_player.h"
#include "L_level.h"
#include "L_log.h"
#include "L_memory.h"

static struct Player players[MAX_PLAYERS] = {0};
static struct Player* ready_players = NULL;
static struct Player* active_players = NULL;

static SDL_PropertiesID default_player_flags = 0;

void player_init() {
    default_player_flags = SDL_CreateProperties();
    if (default_player_flags == 0)
        FATAL("Default player flags fail: %s", SDL_GetError());

    for (size_t i = 0; i < MAX_PLAYERS; i++) {
        players[i].slot = i;
        players[i].status = PS_INACTIVE;

        players[i].input.buttons = players[i].last_input.buttons = PB_NONE;

        players[i].userdata = create_pointer_ref("player", &(players[i]));
        players[i].flags = SDL_CreateProperties();
        if (players[i].flags == 0)
            FATAL("Player %u flags fail: %s", i, SDL_GetError());
    }

    activate_player(0);

    INFO("Opened");
}

void player_teardown() {
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
        unreference_pointer(&(players[i].userdata));
        CLOSE_HANDLE(players[i].flags, SDL_DestroyProperties);
        if (players[i].room != NULL)
            player_leave_room(&(players[i]));
        if (players[i].actor != NULL)
            destroy_actor(players[i].actor, false, false);
    }
    ready_players = active_players = NULL;

    CLOSE_HANDLE(default_player_flags, SDL_DestroyProperties);

    INFO("Closed");
}

void load_pflags(yyjson_val* root) {
    yyjson_val* flags = yyjson_obj_get(root, "player");
    if (flags != NULL) {
        size_t i, n;
        yyjson_val *key, *value;
        yyjson_obj_foreach(flags, i, n, key, value) {
            const char* name = yyjson_get_str(key);
            switch (yyjson_get_type(value)) {
                default:
                    WARN("Invalid player flag \"%s\" type %s", name, yyjson_get_type_desc(value));
                    break;

                case YYJSON_TYPE_BOOL:
                    SDL_SetBooleanProperty(default_player_flags, name, yyjson_get_bool(value));
                    break;

                case YYJSON_TYPE_NUM:
                    if (yyjson_is_int(value))
                        SDL_SetNumberProperty(default_player_flags, name, (int)yyjson_get_int(value));
                    else if (yyjson_is_real(value))
                        SDL_SetFloatProperty(default_player_flags, name, (float)yyjson_get_real(value));
                    break;

                case YYJSON_TYPE_STR:
                    SDL_SetStringProperty(default_player_flags, name, yyjson_get_str(value));
                    break;
            }
        }
    }
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

// Neighbors
struct Room* get_player_room(int slot) {
    return IS_VALID_PSLOT(slot) ? players[slot].room : NULL;
}

bool player_leave_room(struct Player* player) {
    struct Room* room = player->room;
    if (room == NULL)
        return false;

    if (player->actor != NULL)
        destroy_actor(player->actor, false, false);

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

    respawn_player(player);
    return true;
}

struct Actor* respawn_player(struct Player* player) {
    struct Room* room = player->room;
    if (room == NULL)
        return NULL;

    // Kill old pawn
    if (player->actor != NULL)
        destroy_actor(player->actor, false, false);

    // Spawn scan
    struct Actor* spawn = room->actors;
    while (spawn != NULL) {
        if (actor_is_ancestor(spawn, "PlayerSpawn"))
            break;
        spawn = spawn->previous_neighbor;
    }
    if (spawn == NULL)
        return NULL;

    // New pawn
    const char* class = get_pflag_string(player, "class", NULL);
    if (class == NULL)
        return NULL;
    struct Actor* actor = create_actor(room, NULL, class, false, spawn->pos, spawn->angle, spawn->tag);
    if (actor == NULL)
        return NULL;
    actor->player = player;
    player->actor = actor;
    if (actor->type->create != LUA_NOREF)
        execute_ref_in(actor->type->create, actor->userdata, actor->type->name);
    actor->flags &= ~AF_NEW;
    return actor;
}

// Flags
enum FlagTypes get_pflag_type(struct Player* player, const char* name) {
    switch (SDL_HasProperty(player->flags, name) ? SDL_GetPropertyType(player->flags, name)
                                                 : SDL_GetPropertyType(default_player_flags, name)) {
        default:
            return FT_NULL;
        case SDL_PROPERTY_TYPE_BOOLEAN:
            return FT_BOOL;
        case SDL_PROPERTY_TYPE_NUMBER:
            return FT_INT;
        case SDL_PROPERTY_TYPE_FLOAT:
            return FT_FLOAT;
        case SDL_PROPERTY_TYPE_STRING:
            return FT_STRING;
    }
}

bool get_pflag_bool(struct Player* player, const char* name, bool failsafe) {
    return SDL_HasProperty(player->flags, name) ? SDL_GetBooleanProperty(player->flags, name, failsafe)
                                                : SDL_GetBooleanProperty(default_player_flags, name, failsafe);
}

int get_pflag_int(struct Player* player, const char* name, int failsafe) {
    return (int)(SDL_HasProperty(player->flags, name) ? SDL_GetNumberProperty(player->flags, name, failsafe)
                                                      : SDL_GetNumberProperty(default_player_flags, name, failsafe));
}

float get_pflag_float(struct Player* player, const char* name, float failsafe) {
    return SDL_HasProperty(player->flags, name) ? SDL_GetFloatProperty(player->flags, name, failsafe)
                                                : SDL_GetFloatProperty(default_player_flags, name, failsafe);
}

const char* get_pflag_string(struct Player* player, const char* name, const char* failsafe) {
    return SDL_HasProperty(player->flags, name) ? SDL_GetStringProperty(player->flags, name, failsafe)
                                                : SDL_GetStringProperty(default_player_flags, name, failsafe);
}

void clear_pflags(struct Player* player) {
    SDL_DestroyProperties(player->flags);
    player->flags = SDL_CreateProperties();
    if (player->flags == 0)
        FATAL("clear_pflags failed in player %u: %s", player->slot, SDL_GetError());
}

void delete_pflag(struct Player* player, const char* name) {
    SDL_ClearProperty(player->flags, name);
}

void set_pflag_bool(struct Player* player, const char* name, bool value) {
    SDL_SetBooleanProperty(player->flags, name, value);
}

void set_pflag_int(struct Player* player, const char* name, int value) {
    SDL_SetNumberProperty(player->flags, name, value);
}

void set_pflag_float(struct Player* player, const char* name, float value) {
    SDL_SetFloatProperty(player->flags, name, value);
}

void set_pflag_string(struct Player* player, const char* name, const char* value) {
    SDL_SetStringProperty(player->flags, name, value);
}
