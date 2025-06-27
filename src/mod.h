#pragma once

#include <SDL3/SDL_filesystem.h>

#include "file.h"
#include "mem.h"

#define MOD_NAME_MAX 128

typedef HandleID ModID;

struct Mod {
    ModID hid;
    char name[FILE_NAME_MAX];
    char path[FILE_PATH_MAX];
    uint32_t crc32;

    char title[MOD_NAME_MAX];
    uint16_t version;

    struct Mod *previous, *next;
};

void mod_init();
void mod_init_script();
void mod_init_language();
void mod_teardown();

struct Mod* get_mod(const char*);
ModID get_mod_hid(const char*);
struct Mod* hid_to_mod(ModID);

const char* get_mod_file(const char*, const char*);
