#pragma once

#include <SDL3/SDL_properties.h>

void config_init(const char*, const char*);
void config_teardown();

bool get_bool_cvar(const char*);
Sint64 get_int_cvar(const char*);
float get_float_cvar(const char*);
const char* get_string_cvar(const char*);

void set_bool_cvar(const char*, bool);
void set_int_cvar(const char*, Sint64);
void set_float_cvar(const char*, float);
void set_string_cvar(const char*, const char*);

bool reset_cvar(const char*);
void apply_cvar(const char*);

void save_config();
void load_config();
