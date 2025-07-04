#include "L_room.h"

void destroy_room(struct Room* room) {
    struct Player* player = room->players;
    while (player != NULL) {
        struct Player* it = player->previous_neighbor;
        player_leave_room(player);
        player = it;
    }

    lame_free(&room);
}
