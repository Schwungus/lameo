#pragma once

#include "L_room.h"

struct Level {
    const char *name, *title;
    struct Room* rooms;
};
