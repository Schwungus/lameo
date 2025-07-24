#include <SDL3/SDL_properties.h>

#include "L_flags.h"
#include "L_log.h"
#include "L_memory.h"

static SDL_PropertiesID static_flags = 0;
static SDL_PropertiesID default_global_flags = 0;
static SDL_PropertiesID global_flags = 0;
static SDL_PropertiesID local_flags = 0;

void flags_init() {
    static_flags = SDL_CreateProperties();
    if (static_flags == 0)
        FATAL("Static flags fail: %s", SDL_GetError());

    default_global_flags = SDL_CreateProperties();
    if (default_global_flags == 0)
        FATAL("Default global flags fail: %s", SDL_GetError());

    global_flags = SDL_CreateProperties();
    if (global_flags == 0)
        FATAL("Global flags fail: %s", SDL_GetError());

    local_flags = SDL_CreateProperties();
    if (local_flags == 0)
        FATAL("Local flags fail: %s", SDL_GetError());

    INFO("Opened");
}

void flags_teardown() {
    CLOSE_HANDLE(static_flags, SDL_DestroyProperties);
    CLOSE_HANDLE(default_global_flags, SDL_DestroyProperties);
    CLOSE_HANDLE(global_flags, SDL_DestroyProperties);
    CLOSE_HANDLE(local_flags, SDL_DestroyProperties);

    INFO("Closed");
}

void load_flags(yyjson_val* root) {
    yyjson_val* flags = yyjson_obj_get(root, "static");
    if (flags != NULL) {
        size_t i, n;
        yyjson_val *key, *value;
        yyjson_obj_foreach(flags, i, n, key, value) {
            const char* name = yyjson_get_str(key);
            switch (yyjson_get_type(value)) {
                default:
                    WARN("Invalid static flag \"%s\" type %s", name, yyjson_get_type_desc(value));
                    break;

                case YYJSON_TYPE_BOOL:
                    SDL_SetBooleanProperty(static_flags, name, yyjson_get_bool(value));
                    break;

                case YYJSON_TYPE_NUM:
                    if (yyjson_is_int(value))
                        SDL_SetNumberProperty(static_flags, name, (int)yyjson_get_int(value));
                    else if (yyjson_is_real(value))
                        SDL_SetFloatProperty(static_flags, name, (float)yyjson_get_real(value));
                    break;

                case YYJSON_TYPE_STR:
                    SDL_SetStringProperty(static_flags, name, yyjson_get_str(value));
                    break;
            }
        }
    }

    flags = yyjson_obj_get(root, "global");
    if (flags != NULL) {
        size_t i, n;
        yyjson_val *key, *value;
        yyjson_obj_foreach(flags, i, n, key, value) {
            const char* name = yyjson_get_str(key);
            switch (yyjson_get_type(value)) {
                default:
                    WARN("Invalid global flag \"%s\" type %s", name, yyjson_get_type_desc(value));
                    break;

                case YYJSON_TYPE_BOOL:
                    SDL_SetBooleanProperty(default_global_flags, name, yyjson_get_bool(value));
                    break;

                case YYJSON_TYPE_NUM:
                    if (yyjson_is_int(value))
                        SDL_SetNumberProperty(default_global_flags, name, (int)yyjson_get_int(value));
                    else if (yyjson_is_real(value))
                        SDL_SetFloatProperty(default_global_flags, name, (float)yyjson_get_real(value));
                    break;

                case YYJSON_TYPE_STR:
                    SDL_SetStringProperty(default_global_flags, name, yyjson_get_str(value));
                    break;
            }
        }
    }
}

FLAGS_READ_SOURCE(static);

enum FlagTypes get_global_type(const char* name) {
    switch (SDL_HasProperty(global_flags, name) ? SDL_GetPropertyType(global_flags, name)
                                                : SDL_GetPropertyType(default_global_flags, name)) {
        default:
            return FT_NULL;
        case SDL_PROPERTY_TYPE_BOOLEAN:
            return FT_BOOL;
        case SDL_PROPERTY_TYPE_NUMBER:
            return FT_INT;
        case SDL_PROPERTY_TYPE_FLOAT:
            return FT_FLOAT;
        case SDL_PROPERTY_TYPE_STRING:
            return FT_STRING;
    }
}

bool get_global_bool(const char* name, bool failsafe) {
    return SDL_HasProperty(global_flags, name) ? SDL_GetBooleanProperty(global_flags, name, failsafe)
                                               : SDL_GetBooleanProperty(default_global_flags, name, failsafe);
}

int get_global_int(const char* name, int failsafe) {
    return (int)(SDL_HasProperty(global_flags, name) ? SDL_GetNumberProperty(global_flags, name, failsafe)
                                                     : SDL_GetNumberProperty(default_global_flags, name, failsafe));
}

float get_global_float(const char* name, float failsafe) {
    return SDL_HasProperty(global_flags, name) ? SDL_GetFloatProperty(global_flags, name, failsafe)
                                               : SDL_GetFloatProperty(default_global_flags, name, failsafe);
}

const char* get_global_string(const char* name, const char* failsafe) {
    return SDL_HasProperty(global_flags, name) ? SDL_GetStringProperty(global_flags, name, failsafe)
                                               : SDL_GetStringProperty(default_global_flags, name, failsafe);
}

FLAGS_WRITE_SOURCE(global);

FLAGS_READ_SOURCE(local);
FLAGS_WRITE_SOURCE(local);
