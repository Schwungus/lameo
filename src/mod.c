#include <stdio.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_filesystem.h>

#include "mod.h"
#include "mem.h"
#include "log.h"

static struct Mod* mods = NULL;
static SDL_PropertiesID disabled = 0;

SDL_EnumerationResult iterate_mods(void* userdata, const char* dirname, const char* fname) {
    char path[MOD_PATH_MAX];
    SDL_snprintf(path, sizeof(path), "%s%s/", dirname, fname);

    // Mods are folders so skip anything that isn't
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_DIRECTORY)
        return SDL_ENUM_CONTINUE;

    // Skip disabled mods
    if (SDL_GetBooleanProperty(disabled, fname, false)) {
        INFO("Skipping disabled mod \"%s\"", fname);
        return SDL_ENUM_CONTINUE;
    }

    // Alloc
    struct Mod* mod = lame_alloc(sizeof(*mod));
    SDL_strlcpy(mod->name, fname, MOD_NAME_MAX);
    SDL_strlcpy(mod->path, path, MOD_PATH_MAX);

    uint32_t crc32 = 0;
    if (!SDL_EnumerateDirectory(path, iterate_crc32, &crc32))
        FATAL("CRC32 fail: %s", SDL_GetError());
    mod->crc32 = crc32;

    mod->previous = NULL;

    // Push
    if (mods != NULL)
        mod->previous = mods;
    mods = mod;
    SDL_SetBooleanProperty(disabled, fname, false);

    INFO("Added mod \"%s\" (%u)", mod->name, mod->crc32);
    return SDL_ENUM_CONTINUE;
}

SDL_EnumerationResult iterate_crc32(void* userdata, const char* dirname, const char* fname) {
    char path[MOD_PATH_MAX];
    SDL_snprintf(path, sizeof(path), "%s%s", dirname, fname);

    // Recursive into directories
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        // Add folder name to hash so they actually matter
        *(uint32_t*)userdata += SDL_crc32(*(uint32_t*)userdata, fname, SDL_strlen(fname));
        return SDL_EnumerateDirectory(path, iterate_crc32, userdata) ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
    }

    // Add file to hash
    size_t size;
    void* buffer = SDL_LoadFile(path, &size);
    if (buffer == NULL)
        FATAL("CRC32 fail: %s", SDL_GetError());
    *(uint32_t*)userdata += SDL_crc32(*(uint32_t*)userdata, buffer, size);
    lame_free(&buffer);

    return SDL_ENUM_CONTINUE;
}

void mod_init() {
    // Load enabled mods
    disabled = SDL_CreateProperties();
    if (disabled == 0)
        FATAL("Mod list fail: %s", SDL_GetError());

    FILE* disfile = fopen("data/disable.txt", "r");
    if (disfile != NULL) {
        char line[MOD_NAME_MAX];
        while (fgets(line, sizeof(line), disfile) != NULL) // i don't suppose there's an SDL equivalent
            if (line[0] != '\0' && line[0] != '\n') {
                size_t len = SDL_strlen(line);
                if (line[len - 1] == '\n')
                    line[len - 1] = '\0';

                SDL_SetBooleanProperty(disabled, line, true);
            }

        fclose(disfile);
    }

    if (!SDL_EnumerateDirectory("data/", iterate_mods, NULL))
        FATAL("Mod load fail: %s", SDL_GetError());

    // Check internal mods
    if (get_mod("0main") == NULL)
        FATAL("Internal mod \"0main\" not found");

    INFO("Opened");
}

void mod_teardown() {
    while (mods != NULL) {
        struct Mod* mod = mods;
        mods = mod->previous;
        INFO("Removed mod \"%s\" (%u)", mod->name, mod->crc32);
        lame_free(&mod);
    }

    CLOSE_HANDLE(disabled, SDL_DestroyProperties);

    INFO("Closed");
}

struct Mod* get_mod(const char* name) {
    for (struct Mod* mod = mods; mod != NULL; mod = mod->previous) {
        if (SDL_strcmp(mod->name, name) == 0)
            return mod;
    }

    return NULL;
}

static char path_result[MOD_PATH_MAX];
const char* get_file(const char* filename, const char* exclude_ext) {
    for (struct Mod* mod = mods; mod != NULL; mod = mod->previous) {
        int count;
        char** files = SDL_GlobDirectory(mod->path, filename, 0, &count);
        if (files == NULL)
            continue;

        int success = 0;
        for (int i = 0; i < count; i++) {
            const char *file = files[i];

            if (exclude_ext != NULL) {
                const char* ext = SDL_strrchr(file, '.');

                if (ext != NULL && SDL_strcmp(ext, exclude_ext) == 0)
                    continue;
            }

            SDL_snprintf(path_result, sizeof(path_result), "%s%s", mod->path, file);
            success = 1;
            break;
        }
        lame_free(&files);

        if (success)
            return path_result;
    }

    return NULL;
}
