#include "L_localize.h"
#include "L_log.h"
#include "L_mod.h"

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

        destroy_hash_map(lang->map);
        lame_free(&lang);

        lang = temp;
    }

    INFO("Closed");
}

struct Language* fetch_language(const char* name) {
    struct Language* lang = languages;
    for (; lang != NULL; lang = lang->previous)
        if (SDL_strcmp(lang->name, name) == 0)
            return lang;

    lang = lame_alloc(sizeof(struct Language));
    SDL_strlcpy(lang->name, name, LANGUAGE_NAME_MAX);
    lang->map = create_hash_map();

    if (languages != NULL)
        languages->next = lang;
    lang->previous = languages;
    lang->next = NULL;
    languages = lang;

    INFO("Added language \"%s\"", name);

    return lang;
}

void set_default_language(struct Language* lang) {
    default_language = lang;
}

void set_language(const char* name) {
    for (struct Language* lang = languages; lang != NULL; lang = lang->previous)
        if (SDL_strcmp(lang->name, name) == 0) {
            language = lang;
            INFO("Language set to %s", name);
            return;
        }
}

static const char* get_local_string(struct Language* lang, const char* key) {
    if (lang == NULL)
        return NULL;
    return (const char*)from_hash_map(lang->map, key);
}

const char* localized(const char* key) {
    const char* str = get_local_string(language, key);
    if (str == NULL && language != default_language)
        str = get_local_string(default_language, key);

    return str == NULL ? key : str;
}
