#include "localize.h"
#include "log.h"
#include "mem.h"
#include "mod.h"

static struct Language* languages = NULL;
static struct Language *language = NULL, *default_language = NULL;

void localize_init() {
    mod_init_language();

    INFO("Opened");
}

void localize_teardown() {
    struct Language* lang = languages;
    while (lang != NULL) {
        struct Language* temp = lang->previous;

        for (size_t i = 0; i < lang->string_capacity; i++) {
            struct LocalString* string = &lang->strings[i];
            FREE_POINTER(string->key);
            FREE_POINTER(string->string);
        }
        lame_free(&lang->strings);
        lame_free(&lang);

        lang = temp;
    }

    INFO("Closed");
}

// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
#define FNV_OFFSET 0x811c9dc5
#define FNV_PRIME 0x01000193

static uint32_t hash_key(const char* key) {
    uint32_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint32_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

#define HASH_CAPACITY 16

struct Language* fetch_language(const char* name) {
    struct Language* lang = languages;
    for (; lang != NULL; lang = lang->previous)
        if (SDL_strcmp(lang->name, name) == 0)
            return lang;

    lang = lame_alloc(sizeof(struct Language));
    SDL_strlcpy(lang->name, name, LANGUAGE_NAME_MAX);

    lang->string_count = 0;
    lang->string_capacity = HASH_CAPACITY;

    const size_t size = lang->string_capacity * sizeof(struct LocalString);
    lang->strings = lame_alloc(size);
    lame_set(lang->strings, 0, size);

    if (languages != NULL)
        languages->next = lang;
    lang->previous = languages;
    lang->next = NULL;
    languages = lang;

    INFO("Added language \"%s\"", name);

    return lang;
}

void set_local_string(struct Language* lang, const char* key, const char* str) {
    if (lang->string_count >= lang->string_capacity / 2) {
        size_t new_capacity = lang->string_capacity * 2;
        if (new_capacity < lang->string_capacity)
            FATAL("You're setting too many strings in %s, calm down...", lang->name);

        size_t old_capacity = lang->string_capacity;
        lame_realloc(&lang->strings, new_capacity);
        lame_set(lang->strings + old_capacity, 0, (new_capacity - old_capacity) * sizeof(struct LocalString));
        lang->string_capacity = new_capacity;
    }

    size_t index = (size_t)hash_key(key) % lang->string_capacity;
    struct LocalString* string = &lang->strings[index];
    while (string->key != NULL) {
        if (SDL_strcmp(key, string->key) == 0) {
            lame_free(&string->string);
            string->string = SDL_strdup(str);
            return;
        }

        index = (index + 1) % lang->string_capacity;
        string = &lang->strings[index];
    }

    lang->strings[index].key = SDL_strdup(key);
    lang->strings[index].string = SDL_strdup(str);
}

void set_default_language(struct Language* lang) {
    default_language = lang;
}

static const char* get_local_string(struct Language* lang, const char* key) {
    if (lang == NULL)
        return NULL;

    size_t index = (size_t)hash_key(key) % lang->string_capacity;
    struct LocalString* string = &lang->strings[index];
    while (string->key != NULL) {
        if (SDL_strcmp(key, string->key) == 0)
            return string->string;

        index = (index + 1) % lang->string_capacity;
        string = &lang->strings[index];
    }

    return NULL;
}

const char* localized(const char* key) {
    const char* str = get_local_string(language, key);
    if (str == NULL && language != default_language)
        str = get_local_string(default_language, key);

    return str == NULL ? key : str;
}
