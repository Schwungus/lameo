#pragma once

#include <SDL3/SDL_events.h>

#include "player.h"

#define MAX_INPUT_PLAYERS MAX_PLAYERS
#define VERB_NAME_MAX 32
#define VERB_VALUE_MAX 32767

#define NO_KEY SDL_SCANCODE_UNKNOWN
#define NO_MOUSE_BUTTON 0
#define NO_GAMEPAD_BUTTON SDL_GAMEPAD_BUTTON_INVALID

#define KEY_COUNT SDL_SCANCODE_COUNT
#define MOUSE_BUTTON_COUNT 6 // Last entry is SDL_BUTTON_X2 (why aren't these enums?)
#define GAMEPAD_BUTTON_COUNT SDL_GAMEPAD_BUTTON_COUNT

enum Verbs {
    // Game
    VERB_UP,
    VERB_LEFT,
    VERB_DOWN,
    VERB_RIGHT,
    VERB_WALK,
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

    // UI
    VERB_UI_UP,
    VERB_UI_LEFT,
    VERB_UI_DOWN,
    VERB_UI_RIGHT,
    VERB_UI_ENTER,
    VERB_UI_CLICK,
    VERB_UI_BACK,

    // Misc
    VERB_PAUSE,
    VERB_LEAVE,

    // Debug
    VERB_DEBUG_FPS,
    VERB_DEBUG_CONSOLE,
    VERB_DEBUG_CONSOLE_SUBMIT,
    VERB_DEBUG_CONSOLE_PREVIOUS,

    VERB_SIZE,
    VERB_NULL = VERB_SIZE,
};

struct Verb {
    char name[VERB_NAME_MAX];

    SDL_Scancode key, default_key;
    uint8_t mouse_button, default_mouse_button;
    SDL_GamepadButton gamepad_button, default_gamepad_button;
    int8_t gamepad_axis, default_gamepad_axis;

    bool held;
    int16_t value;
};

struct VerbList {
    struct Verb** list;
    size_t size;
};

void input_init();
void input_update();
void input_teardown();

void define_verb(enum Verbs, const char*, SDL_Scancode, uint8_t, SDL_GamepadButton, int8_t);
void assign_verb_to_key(struct Verb*, SDL_Scancode);
void assign_verb_to_mouse_button(struct Verb*, uint8_t);
void assign_verb_to_gamepad_button(struct Verb*, SDL_GamepadButton);
inline struct Verb* get_verb(enum Verbs);

void handle_keyboard(SDL_KeyboardDeviceEvent*);
void handle_key(SDL_KeyboardEvent*);

void handle_mouse(SDL_MouseDeviceEvent*);
void handle_mouse_button(SDL_MouseButtonEvent*);
/*void handle_mouse_wheel(SDL_MouseWheelEvent*);
void handle_mouse_motion(SDL_MouseMotionEvent*);*/

/*void handle_gamepad(SDL_GamepadDeviceEvent*);
void handle_gamepad_button(SDL_GamepadButtonEvent*);
void handle_gamepad_axis(SDL_GamepadAxisEvent*);
void handle_gamepad_sensor(SDL_GamepadSensorEvent*);
void handle_gamepad_touchpad(SDL_GamepadTouchpadEvent*);*/

inline int16_t input_value(enum Verbs, size_t);
