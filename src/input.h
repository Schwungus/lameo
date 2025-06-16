#pragma once

#include <SDL3/SDL_events.h>

#include "player.h"

#define MAX_INPUT_PLAYERS MAX_PLAYERS

enum Verbs {
    VERB_UP,
    VERB_LEFT,
    VERB_DOWN,
    VERB_RIGHT,
    VERB_JUMP,
    VERB_INTERACT,
    VERB_ATTACK,
    VERB_INVENTORY1,
    VERB_INVENTORY2,
    VERB_INVENTORY3,
    VERB_INVENTORY4,
    VERB_AIM,
    VERB_AIM_UP,
    VERB_AIM_LEFT,
    VERB_AIM_DOWN,
    VERB_AIM_RIGHT,
    VERB_PAUSE,
    VERB_LEAVE,
    VERB_DEBUG_FPS,
    VERB_DEBUG_CONSOLE,
    VERB_DEBUG_CONSOLE_SUBMIT,
    VERB_DEBUG_CONSOLE_PREVIOUS,
    VERB_SIZE,
};

struct Verb {
    char name[32];
    SDL_Keycode key, default_key;
    uint8_t mouse_button, default_mouse_button;
    SDL_GamepadButton gamepad_button, default_gamepad_button;
    int8_t gamepad_axis, default_gamepad_axis;
};

struct InputPlayer {
    uint8_t slot;
    bool active;
    enum InputSources source;
};

enum InputModes {
    INM_FIXED,
    INM_HOTSWAP,
    INM_MULTIPLAYER,
};

enum InputSources {
    INS_KEYBOARD_MOUSE,
    INS_GAMEPAD,
};

void define_verb(enum Verbs, SDL_Keycode, uint8_t, SDL_GamepadButton, int8_t);
