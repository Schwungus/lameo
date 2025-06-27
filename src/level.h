#pragma once

#include "room.h"

#define LEVEL_NAME_MAX 128
#define LEVEL_TITLE_MAX 128
#define LEVEL_ICON_MAX 128

struct Level {
    char name[LEVEL_NAME_MAX], title[LEVEL_TITLE_MAX];
    char icon[LEVEL_ICON_MAX];
    struct Room* rooms;
};
