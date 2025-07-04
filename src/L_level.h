#pragma once

#include "L_memory.h"
#include "L_room.h" // IWYU pragma: keep

struct Level {
    const char *name, *title;
    struct IntMap* rooms;
};

void unload_level();
void load_level(const char*, uint32_t, uint16_t);
void go_to_level(const char*, uint32_t, uint16_t);
struct Level* get_level();
