#include <SDL3/SDL_stdinc.h>

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
    struct Language* language = languages;
    while (language != NULL) {
        struct Language* temp = language->previous;

        for (size_t i = 0; i < language->string_capacity; i++) {
            struct LocalString* string = &language->strings[i];
            if (string->key != NULL)
                lame_free(&string->key);
            if (string->string != NULL)
                lame_free(&string->string);
        }
        lame_free(&language->strings);
        lame_free(&language);

        language = temp;
    }

    INFO("Closed");
}

struct Language* fetch_language(const char* name) {
    for (struct Language* language = languages; language != NULL; language = language->previous)
        if (SDL_strcmp(language->name, name) == 0)
            return language;

    language = lame_alloc(sizeof(struct Language));
    SDL_strlcpy(language->name, name, LANGUAGE_NAME_MAX);

    language->string_count = 0;
    language->string_capacity = 1;
    language->strings = lame_alloc(language->string_capacity * sizeof(struct LocalString));
    lame_set(language->strings, 0, language->string_capacity * sizeof(struct LocalString));

    if (languages != NULL)
        languages->next = language;
    language->previous = languages;
    language->next = NULL;
    languages = language;

    INFO("Added language \"%s\"", name);

    return language;
}

void set_default_language(struct Language* language) {
    default_language = language;
}
