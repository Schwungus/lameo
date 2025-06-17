#pragma once

#include <SDL3/SDL_filesystem.h>

#include "mem.h"

#define MOD_NAME_MAX 128
#define MOD_PATH_MAX 256

typedef HandleID ModID;

struct Mod {
    ModID hid;
    char name[MOD_NAME_MAX];
    char path[MOD_PATH_MAX];
    uint32_t crc32;

    char title[MOD_NAME_MAX];
    uint16_t version;

    struct Mod *previous, *next;
};

void mod_init();
void mod_init_script();
void mod_teardown();

struct Mod* get_mod(const char*);
ModID get_mod_hid(const char*);
inline struct Mod* hid_to_mod(ModID);

const char* get_file(const char*, const char*);
