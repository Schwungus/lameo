#pragma once

#include "L_file.h" // IWYU pragma: keep

#define FLAGS_READ_HEADER(scope)                                                                                       \
    enum FlagTypes get_##scope##_type(const char*);                                                                    \
    bool get_##scope##_bool(const char*, bool);                                                                        \
    int get_##scope##_int(const char*, int);                                                                           \
    float get_##scope##_float(const char*, float);                                                                     \
    const char* get_##scope##_string(const char*, const char*);

#define FLAGS_WRITE_HEADER(scope)                                                                                      \
    void clear_##scope();                                                                                              \
    void delete_##scope(const char*);                                                                                  \
    void set_##scope##_bool(const char*, bool);                                                                        \
    void set_##scope##_int(const char*, int);                                                                          \
    void set_##scope##_float(const char*, float);                                                                      \
    void set_##scope##_string(const char*, const char*);

#define FLAGS_READ_SOURCE(scope)                                                                                       \
    enum FlagTypes get_##scope##_type(const char* name) {                                                              \
        switch (SDL_GetPropertyType(scope##_flags, name)) {                                                            \
            default:                                                                                                   \
                return FT_NULL;                                                                                        \
            case SDL_PROPERTY_TYPE_BOOLEAN:                                                                            \
                return FT_BOOL;                                                                                        \
            case SDL_PROPERTY_TYPE_NUMBER:                                                                             \
                return FT_INT;                                                                                         \
            case SDL_PROPERTY_TYPE_FLOAT:                                                                              \
                return FT_FLOAT;                                                                                       \
            case SDL_PROPERTY_TYPE_STRING:                                                                             \
                return FT_STRING;                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    bool get_##scope##_bool(const char* name, bool failsafe) {                                                         \
        return SDL_GetBooleanProperty(scope##_flags, name, failsafe);                                                  \
    }                                                                                                                  \
                                                                                                                       \
    int get_##scope##_int(const char* name, int failsafe) {                                                            \
        return (int)SDL_GetNumberProperty(scope##_flags, name, failsafe);                                              \
    }                                                                                                                  \
                                                                                                                       \
    float get_##scope##_float(const char* name, float failsafe) {                                                      \
        return SDL_GetFloatProperty(scope##_flags, name, failsafe);                                                    \
    }                                                                                                                  \
                                                                                                                       \
    const char* get_##scope##_string(const char* name, const char* failsafe) {                                         \
        return SDL_GetStringProperty(scope##_flags, name, failsafe);                                                   \
    }

#define FLAGS_WRITE_SOURCE(scope)                                                                                      \
    void clear_##scope() {                                                                                             \
        SDL_DestroyProperties(scope##_flags);                                                                          \
        scope##_flags = SDL_CreateProperties();                                                                        \
        if (scope##_flags == 0)                                                                                        \
            FATAL("clear_" #scope " fail: %s", SDL_GetError());                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void delete_##scope(const char* name) {                                                                            \
        SDL_ClearProperty(scope##_flags, name);                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void set_##scope##_bool(const char* name, bool value) {                                                            \
        SDL_SetBooleanProperty(scope##_flags, name, value);                                                            \
    }                                                                                                                  \
                                                                                                                       \
    void set_##scope##_int(const char* name, int value) {                                                              \
        SDL_SetNumberProperty(scope##_flags, name, value);                                                             \
    }                                                                                                                  \
                                                                                                                       \
    void set_##scope##_float(const char* name, float value) {                                                          \
        SDL_SetFloatProperty(scope##_flags, name, value);                                                              \
    }                                                                                                                  \
                                                                                                                       \
    void set_##scope##_string(const char* name, const char* value) {                                                   \
        SDL_SetStringProperty(scope##_flags, name, value);                                                             \
    }

enum FlagTypes {
    FT_NULL = SDL_PROPERTY_TYPE_INVALID,
    FT_BOOL = SDL_PROPERTY_TYPE_BOOLEAN,
    FT_INT = SDL_PROPERTY_TYPE_NUMBER,
    FT_FLOAT = SDL_PROPERTY_TYPE_FLOAT,
    FT_STRING = SDL_PROPERTY_TYPE_STRING,
};

void flags_init();
void flags_teardown();
void load_flags(yyjson_val*);

FLAGS_READ_HEADER(static);

FLAGS_READ_HEADER(global);
FLAGS_WRITE_HEADER(global);

FLAGS_READ_HEADER(local);
FLAGS_WRITE_HEADER(local);
