#pragma once

#include "L_memory.h"

typedef HandleID ModID;

struct Mod {
    ModID hid;
    const char *name, *title;
    const char* path;
    uint32_t crc32;
    uint16_t version;

    struct Mod *previous, *next; // Position in list (previous-order)
};

void mod_init();
void mod_init_script();
void mod_init_language();
void mod_teardown();

struct Mod* get_mod(const char*);
ModID get_mod_hid(const char*);
struct Mod* hid_to_mod(ModID);

const char* get_mod_file(const char*, const char*);
