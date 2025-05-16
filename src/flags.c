#include "flags.h"
#include "log.h"
#include "mem.h"

static FlagsID flags[FS_SIZE] = {0};

void flags_init() {
    for (int i = 0; i < FS_SIZE; i++)
        if ((flags[i] = (FlagsID)SDL_CreateProperties()) == 0)
            FATAL("Flags init fail (%u): %s", i, SDL_GetError());

    INFO("Opened");
}

void flags_teardown() {
    for (int i = 0; i < FS_SIZE; i++)
        SDL_DestroyProperties(flags[i]);

    INFO("Closed");
}

// Flag getters
extern enum FlagTypes get_flag_type(enum FlagScopes scope, const char* name) {
    return IS_INVALID_SCOPE(scope) ? FT_NULL : SDL_GetPropertyType((SDL_PropertiesID)flags[scope], name);
}

extern bool get_bool_flag(enum FlagScopes scope, const char* name, bool default_value) {
    return IS_INVALID_SCOPE(scope) ? default_value
                                   : SDL_GetBooleanProperty((SDL_PropertiesID)flags[scope], name, default_value);
}

extern Sint64 get_int_flag(enum FlagScopes scope, const char* name, Sint64 default_value) {
    return IS_INVALID_SCOPE(scope) ? default_value
                                   : SDL_GetNumberProperty((SDL_PropertiesID)flags[scope], name, default_value);
}

extern float get_float_flag(enum FlagScopes scope, const char* name, float default_value) {
    return IS_INVALID_SCOPE(scope) ? default_value
                                   : SDL_GetFloatProperty((SDL_PropertiesID)flags[scope], name, default_value);
}

extern const char* get_string_flag(enum FlagScopes scope, const char* name, const char* default_value) {
    return IS_INVALID_SCOPE(scope) ? default_value
                                   : SDL_GetStringProperty((SDL_PropertiesID)flags[scope], name, default_value);
}

// Flag setters
extern void set_bool_flag(enum FlagScopes scope, const char* name, bool value) {
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetBooleanProperty((SDL_PropertiesID)flags[scope], name, value);
}

extern void set_int_flag(enum FlagScopes scope, const char* name, Sint64 value) {
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetNumberProperty((SDL_PropertiesID)flags[scope], name, value);
}

extern void set_float_flag(enum FlagScopes scope, const char* name, float value) {
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetFloatProperty((SDL_PropertiesID)flags[scope], name, value);
}

extern void set_string_flag(enum FlagScopes scope, const char* name, const char* value) {
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetStringProperty((SDL_PropertiesID)flags[scope], name, value);
}

// Flag utilities
extern void reset_flag(enum FlagScopes scope, const char* name) {
    if (IS_WRITABLE_SCOPE(scope))
        SDL_ClearProperty((SDL_PropertiesID)flags[scope], name);
}

extern bool toggle_flag(enum FlagScopes scope, const char* name) {
    bool b = IS_INVALID_SCOPE(scope) ? false : SDL_GetBooleanProperty((SDL_PropertiesID)flags[scope], name, false);
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetBooleanProperty((SDL_PropertiesID)flags[scope], name, b = !b);
    return b;
}

extern Sint64 increment_flag(enum FlagScopes scope, const char* name) {
    Sint64 i = IS_INVALID_SCOPE(scope) ? 0 : SDL_GetNumberProperty((SDL_PropertiesID)flags[scope], name, 0);
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetNumberProperty((SDL_PropertiesID)flags[scope], name, ++i);
    return i;
}

extern Sint64 decrement_flag(enum FlagScopes scope, const char* name) {
    Sint64 i = IS_INVALID_SCOPE(scope) ? 0 : SDL_GetNumberProperty((SDL_PropertiesID)flags[scope], name, 0);
    if (IS_WRITABLE_SCOPE(scope))
        SDL_SetNumberProperty((SDL_PropertiesID)flags[scope], name, --i);
    return i;
}
