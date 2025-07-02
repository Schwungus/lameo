#pragma once

#include "L_memory.h"

struct Language {
    const char* name;
    struct Language *previous, *next;
    struct HashMap* map;
};

void localize_init();
void localize_teardown();

struct Language* fetch_language(const char*);
void set_default_language(struct Language*);
void set_language(const char*);

const char* localized(const char*);
