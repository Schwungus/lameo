#pragma once

#include "room.h"

#define LEVEL_NAME_MAX 32
#define LEVEL_TITLE_MAX 128
#define LEVEL_ICON_MAX 128

struct LevelInfo {
    char name[LEVEL_NAME_MAX], title[LEVEL_TITLE_MAX];
    char icon[LEVEL_ICON_MAX];
    struct LevelInfo *previous, *next;
};

struct Level {
    struct LevelInfo* info;
    struct Room* rooms;
};
