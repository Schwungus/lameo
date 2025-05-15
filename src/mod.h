#pragma once

#include <SDL3/SDL_filesystem.h>

#define MOD_NAME_MAX 128
#define MOD_PATH_MAX 256

struct Mod {
    char name[MOD_NAME_MAX];
    char path[MOD_PATH_MAX];
    uint32_t crc32;

    char title[MOD_NAME_MAX];
    uint16_t version;

    struct Mod* previous;
};

SDL_EnumerationResult iterate_mods(void*, const char*, const char*);
SDL_EnumerationResult iterate_crc32(void*, const char*, const char*);
void mod_init();
void mod_teardown();
struct Mod* get_mod(const char*);
const char* get_file(const char*, const char*);
