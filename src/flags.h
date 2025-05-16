#pragma once

#include <SDL3/SDL_properties.h>

typedef SDL_PropertiesID FlagsID;

enum FlagScopes {
    FS_STATIC,
    FS_GLOBAL,
    FS_LOCAL,
    FS_SIZE,
};

#define IS_INVALID_SCOPE(scope) (scope < FS_STATIC || scope >= FS_SIZE)
#define IS_WRITABLE_SCOPE(scope) (scope > FS_STATIC && scope < FS_SIZE)

enum FlagTypes {
    FT_NULL = SDL_PROPERTY_TYPE_INVALID,
    FT_BOOL = SDL_PROPERTY_TYPE_BOOLEAN,
    FT_INT = SDL_PROPERTY_TYPE_NUMBER,
    FT_FLOAT = SDL_PROPERTY_TYPE_FLOAT,
    FT_STRING = SDL_PROPERTY_TYPE_STRING,
    FT_POINTER = SDL_PROPERTY_TYPE_POINTER,
};

void flags_init();
void flags_teardown();

inline enum FlagTypes get_flag_type(enum FlagScopes, const char*);
inline bool get_bool_flag(enum FlagScopes, const char*, bool);
inline Sint64 get_int_flag(enum FlagScopes, const char*, Sint64);
inline float get_float_flag(enum FlagScopes, const char*, float);
inline const char* get_string_flag(enum FlagScopes, const char*, const char*);

inline void set_bool_flag(enum FlagScopes, const char*, bool);
inline void set_int_flag(enum FlagScopes, const char*, Sint64);
inline void set_float_flag(enum FlagScopes, const char*, float);
inline void set_string_flag(enum FlagScopes, const char*, const char*);

inline void reset_flag(enum FlagScopes, const char*);
inline bool toggle_flag(enum FlagScopes, const char*);
inline Sint64 increment_flag(enum FlagScopes, const char*);
inline Sint64 decrement_flag(enum FlagScopes, const char*);
