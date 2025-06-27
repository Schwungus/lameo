#pragma once

#define LANGUAGE_NAME_MAX 128

struct LocalString {
    char *key, *string;
};

struct Language {
    char name[LANGUAGE_NAME_MAX];
    struct Language *previous, *next;

    struct LocalString* strings;
    size_t string_count, string_capacity;
};

void localize_init();
void localize_teardown();

struct Language* fetch_language(const char*);
void set_local_string(struct Language*, const char*, const char*);
void set_default_language(struct Language*);

const char* localized(const char*);
