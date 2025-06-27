#include "file.h"
#include "log.h"
#include "mem.h"

static char* pref_path = NULL;
static char user_path[FILE_PATH_MAX];

void file_init() {
    if ((pref_path = SDL_GetPrefPath("Schwungus", "lameo")) == NULL)
        FATAL("Pref path fail: %s", SDL_GetError());
    SDL_snprintf(user_path, sizeof(user_path), "%s%s", pref_path, "users/local/");

    INFO("Opened");
}

void file_teardown() {
    lame_free(&pref_path);

    INFO("Closed");
}

static char file_helper[FILE_PATH_MAX];
const char* get_base_path(const char* filename) {
    if (filename == NULL)
        return SDL_GetBasePath();
    SDL_snprintf(file_helper, sizeof(file_helper), "%s%s", SDL_GetBasePath(), filename);
    return file_helper;
}

const char* get_pref_path(const char* filename) {
    if (filename == NULL)
        return pref_path;
    SDL_snprintf(file_helper, sizeof(file_helper), "%s%s", pref_path, filename);
    return file_helper;
}

const char* get_user_path(const char* filename) {
    if (filename == NULL)
        return user_path;
    SDL_snprintf(file_helper, sizeof(file_helper), "%s%s", user_path, filename);
    return file_helper;
}

bool is_base_path(const char* path) {
    const char* base_path = SDL_GetBasePath();
    return SDL_strncmp(path, base_path, SDL_strlen(base_path)) == 0;
}

bool is_pref_path(const char* path) {
    return SDL_strncmp(path, pref_path, SDL_strlen(pref_path)) == 0;
}

bool is_user_path(const char* path) {
    return SDL_strncmp(path, user_path, SDL_strlen(user_path)) == 0;
}

yyjson_doc* load_json(const char* filename) {
    size_t size;
    void* buffer = SDL_LoadFile(filename, &size);
    if (buffer == NULL) {
        WTF("\"%s\" load error: %s", filename, SDL_GetError());
        return NULL;
    }

    yyjson_read_err error;
    yyjson_doc* json = yyjson_read_opts(buffer, size, JSON_READ_FLAGS, NULL, &error);
    lame_free(&buffer);
    if (json == NULL)
        WTF("\"%s\" read error: %s", filename, error.msg);

    return json;
}

static yyjson_write_err json_write_error = {0};
const char* save_json(yyjson_mut_doc* json, const char* filename) {
    size_t size;
    char* buffer = yyjson_mut_write_opts(json, JSON_WRITE_FLAGS, NULL, &size, &json_write_error);
    if (buffer == NULL)
        return json_write_error.msg;

    const char* error = SDL_SaveFile(filename, buffer, size) ? NULL : SDL_GetError();
    lame_free(&buffer);
    return error;
}
