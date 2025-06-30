#include "mod.h"
#include "config.h"
#include "file.h"
#include "localize.h"
#include "log.h"
#include "mem.h"
#include "script.h"

static struct Mod* mods = NULL;
static struct Fixture* mod_handles = NULL;

static char mod_file_helper[FILE_PATH_MAX];

static SDL_EnumerationResult iterate_crc32(void* userdata, const char* dirname, const char* fname) {
    char path[FILE_PATH_MAX];
    SDL_snprintf(path, sizeof(path), "%s%s", dirname, fname);

    // Recursive into directories
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        // Add folder name to hash so they actually matter
        *(uint32_t*)userdata = SDL_crc32(*(uint32_t*)userdata, fname, SDL_strlen(fname));
        return SDL_EnumerateDirectory(path, iterate_crc32, userdata) ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
    }

    // Add file to hash
    size_t size;
    void* buffer = SDL_LoadFile(path, &size);
    if (buffer == NULL)
        FATAL("CRC32 fail: %s", SDL_GetError());
    *(uint32_t*)userdata = SDL_crc32(*(uint32_t*)userdata, buffer, size);
    lame_free(&buffer);

    return SDL_ENUM_CONTINUE;
}

static SDL_EnumerationResult iterate_mods(void* userdata, const char* dirname, const char* fname) {
    char path[FILE_PATH_MAX];
    SDL_snprintf(path, sizeof(path), "%s%s/", dirname, fname);

    // Mods are folders so skip anything that isn't
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_DIRECTORY)
        return SDL_ENUM_CONTINUE;

    // Skip disabled mods
    if (userdata != NULL) {
        size_t i, n;
        yyjson_val* value;
        yyjson_arr_foreach((yyjson_val*)userdata, i, n, value) {
            const char* str = yyjson_get_str(value);
            if (str != NULL && SDL_strcmp(fname, str) == 0) {
                INFO("Skipping disabled mod \"%s\"", fname);
                return SDL_ENUM_CONTINUE;
            }
        }
    }

    struct Mod* mod = lame_alloc(sizeof(*mod));

    // Data
    mod->hid = (ModID)create_handle(mod_handles, mod);
    SDL_strlcpy(mod->name, fname, FILE_NAME_MAX);
    SDL_strlcpy(mod->path, path, FILE_PATH_MAX);
    mod->crc32 = 0;
    if (!SDL_EnumerateDirectory(path, iterate_crc32, &mod->crc32))
        FATAL("CRC32 fail: %s", SDL_GetError());

    // Info
    SDL_strlcat(path, "mod.json", FILE_PATH_MAX);
    yyjson_doc* json = load_json(path);
    if (json == NULL) {
        WARN("Failed to open \"mod.json\" for \"%s\"", fname);
        SDL_strlcpy(mod->title, fname, MOD_NAME_MAX);
        mod->version = 0;
    } else {
        yyjson_val* root = yyjson_doc_get_root(json);
        if (yyjson_is_obj(root)) {
            const char* title = yyjson_get_str(yyjson_obj_get(root, "title"));
            SDL_strlcpy(mod->title, title == NULL ? fname : title, MOD_NAME_MAX);
            mod->version = (uint16_t)yyjson_get_uint(yyjson_obj_get(root, "version"));
        } else {
            WTF("Expected root object in \"%s/mod.json\", got %s", fname, yyjson_get_type_desc(root));
        }

        yyjson_doc_free(json);
    }

    // Push
    if (mods != NULL)
        mods->next = mod;
    mod->previous = mods;
    mod->next = NULL;
    mods = mod;

    INFO("Added mod \"%s\" v%u (%u, %s -> %u)", mod->title, mod->version, mod->hid, mod->name, mod->crc32);
    return SDL_ENUM_CONTINUE;
}

void mod_init() {
    mod_handles = create_fixture();

    // Load enabled mods
    const char* data_path = get_string_cvar("data_path");
    char path[FILE_PATH_MAX];
    SDL_snprintf(path, sizeof(path), "%s/disabled.json", data_path);
    yyjson_doc* json = load_json(path);
    yyjson_val* disabled = NULL;
    if (json != NULL) {
        disabled = yyjson_doc_get_root(json);
        if (!yyjson_is_arr(disabled)) {
            WTF("Expected root array in \"disabled.json\", got %s", yyjson_get_type_desc(disabled));
            yyjson_doc_free(json);
            json = NULL;
            disabled = NULL;
        }
    }

    SDL_snprintf(path, sizeof(path), "%s/", data_path);
    if (!SDL_EnumerateDirectory(path, iterate_mods, (void*)disabled))
        FATAL("Mod load fail: %s", SDL_GetError());
    if (json != NULL)
        yyjson_doc_free(json);

    // Check internal mods
    if (get_mod("0main") == NULL)
        FATAL("Internal mod \"0main\" not found");

    INFO("Opened");
}

void mod_init_script() {
    // Load entry scripts from first to last mod
    struct Mod* mod = mods;
    if (mod != NULL) {
        while (mod->previous != NULL)
            mod = mod->previous;

        void* buffer;
        size_t size;
        while (mod != NULL) {
            SDL_snprintf(mod_file_helper, sizeof(mod_file_helper), "%sscript.lua", mod->path);
            if ((buffer = SDL_LoadFile(mod_file_helper, &size)) != NULL) {
                SDL_snprintf(mod_file_helper, sizeof(mod_file_helper), "%s?.lua", mod->path);
                set_import_path(mod_file_helper);
                execute_buffer(buffer, size, mod->name);
                lame_free(&buffer);
            }

            mod = mod->next;
        }
    }

    INFO("Opened for scripting");
}

void mod_init_language() {
    // Load languages from first to last mod
    struct Mod* mod = mods;
    if (mod != NULL) {
        while (mod->previous != NULL)
            mod = mod->previous;

        while (mod != NULL) {
            SDL_snprintf(mod_file_helper, sizeof(mod_file_helper), "%slanguages.json", mod->path);

            yyjson_doc* json = load_json(mod_file_helper);
            if (json != NULL) {
                yyjson_val* root = yyjson_doc_get_root(json);
                if (yyjson_is_obj(root)) {
                    size_t i, n;
                    yyjson_val *key, *val;
                    yyjson_obj_foreach(root, i, n, key, val) {
                        const char* name = yyjson_get_str(key);
                        if (!yyjson_is_obj(val)) {
                            WTF("Expected %s language \"%s\" as object, got %s", mod->name, name,
                                yyjson_get_type_desc(val));
                            continue;
                        }

                        struct Language* language = fetch_language(name);
                        if (SDL_strcmp(name, "English") == 0)
                            set_default_language(language);

                        size_t j, o;
                        yyjson_val *key2, *val2;
                        yyjson_obj_foreach(val, j, o, key2, val2) {
                            const char* keyc = yyjson_get_str(key2);
                            if (!yyjson_is_str(val2)) {
                                WTF("Expected %s language \"%s\" key \"%s\" as string, got %s", mod->name, name, keyc,
                                    yyjson_get_type_desc(val2));
                                continue;
                            }
                            to_hash_map(language->map, keyc, SDL_strdup(yyjson_get_str(val2)));
                        }
                    }
                } else {
                    WTF("Expected %s languages root as object, got %s", mod->name, yyjson_get_type_desc(root));
                }

                yyjson_doc_free(json);
            }

            mod = mod->next;
        }
    }

    INFO("Opened for languages");
}

void mod_teardown() {
    while (mods != NULL) {
        const struct Mod* mod = mods;
        mods = mod->previous;
        INFO("Removed mod \"%s\" (%s)", mod->title, mod->name);
        // destroy_handle(mod_handles, mod->hid);
        lame_free(&mod);
    }

    destroy_fixture(mod_handles);

    INFO("Closed");
}

struct Mod* get_mod(const char* name) {
    for (struct Mod* mod = mods; mod != NULL; mod = mod->previous)
        if (SDL_strcmp(mod->name, name) == 0)
            return mod;
    return NULL;
}

ModID get_mod_hid(const char* name) {
    for (struct Mod* mod = mods; mod != NULL; mod = mod->previous)
        if (SDL_strcmp(mod->name, name) == 0)
            return mod->hid;
    return 0;
}

struct Mod* hid_to_mod(ModID hid) {
    return (struct Mod*)hid_to_pointer(mod_handles, (HandleID)hid);
}

const char* get_mod_file(const char* filename, const char* exclude_ext) {
    for (struct Mod* mod = mods; mod != NULL; mod = mod->previous) {
        int count;
        char** files = SDL_GlobDirectory(mod->path, filename, 0, &count);
        if (files == NULL)
            continue;

        int success = 0;
        for (int i = 0; i < count; i++) {
            const char* file = files[i];

            if (exclude_ext != NULL) {
                const char* ext = SDL_strrchr(file, '.');
                if (ext != NULL && SDL_strcmp(ext, exclude_ext) == 0)
                    continue;
            }

            SDL_snprintf(mod_file_helper, sizeof(mod_file_helper), "%s%s", mod->path, file);
            success = 1;
            break;
        }
        lame_free(&files);

        if (success)
            return mod_file_helper;
    }

    return NULL;
}
