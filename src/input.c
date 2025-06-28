#include "input.h"
#include "file.h"
#include "log.h"
#include "mem.h"

static struct Verb verbs[VERB_SIZE] = {0};
static SDL_PropertiesID verb_map = 0;

static struct VerbList key_map[KEY_COUNT] = {0};
static struct VerbList mouse_button_map[MOUSE_BUTTON_COUNT] = {0};
static struct VerbList gamepad_button_map[GAMEPAD_BUTTON_COUNT] = {0};
static struct VerbList gamepad_axis_map[GAMEPAD_AXIS_COUNT] = {0};

void input_init() {
    if (SDL_AddGamepadMappingsFromFile(get_base_path("gamecontrollerdb.txt")) == -1)
        WTF("Gamepad database fail: %s", SDL_GetError());

    if ((verb_map = SDL_CreateProperties()) == 0)
        FATAL("Verb map fail: %s", SDL_GetError());

    // Internally invalid gamepad buttons are -1
    for (size_t i = 0; i < VERB_SIZE; i++) {
        verbs[i].default_gamepad_button = verbs[i].gamepad_button = NO_GAMEPAD_BUTTON;
        verbs[i].default_gamepad_axis = verbs[i].gamepad_axis = NO_GAMEPAD_AXIS;
    }

    define_verb(VERB_UP, "up", -1, SDL_SCANCODE_W, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTY);
    define_verb(VERB_LEFT, "left", -1, SDL_SCANCODE_A, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTX);
    define_verb(VERB_DOWN, "down", 1, SDL_SCANCODE_S, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTY);
    define_verb(VERB_RIGHT, "right", 1, SDL_SCANCODE_D, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTX);
    define_verb(VERB_WALK, "walk", 0, SDL_SCANCODE_LCTRL, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, NO_GAMEPAD_AXIS);

    define_verb(VERB_JUMP, "jump", 0, SDL_SCANCODE_SPACE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_SOUTH, NO_GAMEPAD_AXIS);
    define_verb(
        VERB_INTERACT, "interact", 0, SDL_SCANCODE_E, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_EAST, NO_GAMEPAD_AXIS
    );
    define_verb(
        VERB_ATTACK, "attack", 0, SDL_SCANCODE_PERIOD, MOUSE_LEFT, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, NO_GAMEPAD_AXIS
    );

    define_verb(
        VERB_INVENTORY1, "inventory1", -1, SDL_SCANCODE_LSHIFT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,
        NO_GAMEPAD_AXIS
    );
    define_verb(
        VERB_INVENTORY2, "inventory2", -1, SDL_SCANCODE_1, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
        NO_GAMEPAD_AXIS
    );
    define_verb(
        VERB_INVENTORY3, "inventory3", 1, SDL_SCANCODE_F, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_DOWN, NO_GAMEPAD_AXIS
    );
    define_verb(
        VERB_INVENTORY4, "inventory4", 1, SDL_SCANCODE_2, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
        NO_GAMEPAD_AXIS
    );

    define_verb(VERB_AIM, "aim", 0, SDL_SCANCODE_COMMA, MOUSE_RIGHT, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, NO_GAMEPAD_AXIS);
    define_verb(
        VERB_AIM_UP, "aim_up", -1, SDL_SCANCODE_UP, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_RIGHTY
    );
    define_verb(
        VERB_AIM_LEFT, "aim_left", -1, SDL_SCANCODE_LEFT, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_RIGHTX
    );
    define_verb(
        VERB_AIM_DOWN, "aim_down", 1, SDL_SCANCODE_DOWN, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_RIGHTY
    );
    define_verb(
        VERB_AIM_RIGHT, "aim_right", 1, SDL_SCANCODE_RIGHT, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_RIGHTX
    );

    define_verb(VERB_UI_UP, "ui_up", -1, SDL_SCANCODE_UP, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTY);
    define_verb(
        VERB_UI_LEFT, "ui_left", -1, SDL_SCANCODE_LEFT, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTX
    );
    define_verb(
        VERB_UI_DOWN, "ui_down", 1, SDL_SCANCODE_DOWN, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTY
    );
    define_verb(
        VERB_UI_RIGHT, "ui_right", 1, SDL_SCANCODE_RIGHT, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, SDL_GAMEPAD_AXIS_LEFTX
    );

    define_verb(
        VERB_UI_ENTER, "ui_enter", 0, SDL_SCANCODE_RETURN, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_SOUTH, NO_GAMEPAD_AXIS
    );
    define_verb(VERB_UI_CLICK, "ui_click", 0, NO_KEY, MOUSE_LEFT, SDL_GAMEPAD_BUTTON_INVALID, NO_GAMEPAD_AXIS);
    define_verb(
        VERB_UI_BACK, "ui_back", 0, SDL_SCANCODE_ESCAPE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_START, NO_GAMEPAD_AXIS
    );

    define_verb(
        VERB_PAUSE, "pause", 0, SDL_SCANCODE_ESCAPE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_START, NO_GAMEPAD_AXIS
    );
    define_verb(
        VERB_LEAVE, "leave", 0, SDL_SCANCODE_BACKSPACE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_BACK, NO_GAMEPAD_AXIS
    );

    define_verb(VERB_DEBUG_FPS, "debug_fps", 0, SDL_SCANCODE_F1, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, NO_GAMEPAD_AXIS);
    define_verb(
        VERB_DEBUG_CONSOLE, "debug_console", 0, SDL_SCANCODE_BACKSLASH, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON,
        NO_GAMEPAD_AXIS
    );

    INFO("Opened");
}

void input_update() {
    for (size_t i = 0; i < VERB_SIZE; i++)
        if (!verbs[i].held)
            verbs[i].value = 0;
}

void input_teardown() {
    CLOSE_HANDLE(verb_map, SDL_DestroyProperties);

    size_t i;
    for (i = 0; i < KEY_COUNT; i++)
        FREE_POINTER(key_map[i].list);
    for (i = 0; i < MOUSE_BUTTON_COUNT; i++)
        FREE_POINTER(mouse_button_map[i].list);
    for (i = 0; i < GAMEPAD_BUTTON_COUNT; i++)
        FREE_POINTER(gamepad_button_map[i].list);
    for (i = 0; i < GAMEPAD_AXIS_COUNT; i++)
        FREE_POINTER(gamepad_axis_map[i].list);

    INFO("Closed");
}

void define_verb(
    enum Verbs verb, const char* name, int8_t axis, SDL_Scancode key, enum MouseButtons mouse_button,
    SDL_GamepadButton gamepad_button, SDL_GamepadAxis gamepad_axis
) {
    SDL_strlcpy(verbs[verb].name, name, VERB_NAME_MAX);
    SDL_SetNumberProperty(verb_map, name, verb);
    verbs[verb].axis = axis;

    assign_verb_to_key(&verbs[verb], verbs[verb].default_key = key);
    assign_verb_to_mouse_button(&verbs[verb], verbs[verb].default_mouse_button = mouse_button);
    assign_verb_to_gamepad_button(&verbs[verb], verbs[verb].default_gamepad_button = gamepad_button);
    assign_verb_to_gamepad_axis(&verbs[verb], verbs[verb].default_gamepad_axis = gamepad_axis);
}

void assign_verb_to_key(struct Verb* verb, SDL_Scancode key) {
    if (key == verb->key)
        return;

    if (verb->key != NO_KEY) {
        const struct VerbList* list = &key_map[verb->key];
        if (list->list != NULL)
            for (size_t i = 0; i < list->size; i++)
                if (list->list[i] == verb) {
                    list->list[i] = NULL;
                    break;
                }

        INFO("Unassigned verb %s from key %d", verb->name, verb->key);
    }

    verb->key = key;
    if (key != NO_KEY) {
        struct VerbList* list = &key_map[key];
        if (list->list == NULL) {
            list->list = lame_alloc(sizeof(struct Verb*));
            list->size = 1;
            list->list[0] = verb;
        } else {
            size_t i = 0;
            while (1) {
                if (i >= list->size) {
                    list->size = i + 1;
                    lame_realloc(&list->list, list->size * sizeof(struct Verb*));
                    list->list[i] = verb;
                    break;
                }
                if (list->list[i] == NULL) {
                    list->list[i] = verb;
                    break;
                }
                ++i;
            }
        }

        INFO("Assigned verb %s to key %d", verb->name, key);
    }
}

void assign_verb_to_mouse_button(struct Verb* verb, enum MouseButtons mouse_button) {
    if (mouse_button == verb->mouse_button)
        return;

    if (verb->mouse_button != NO_MOUSE_BUTTON) {
        const struct VerbList* list = &mouse_button_map[verb->mouse_button];
        if (list->list != NULL)
            for (size_t i = 0; i < list->size; i++)
                if (list->list[i] == verb) {
                    list->list[i] = NULL;
                    break;
                }

        INFO("Unassigned verb %s from mouse button %d", verb->name, verb->mouse_button);
    }

    verb->mouse_button = mouse_button;
    if (mouse_button != NO_MOUSE_BUTTON) {
        struct VerbList* list = &mouse_button_map[mouse_button];
        if (list->list == NULL) {
            list->list = lame_alloc(sizeof(struct Verb*));
            list->size = 1;
            list->list[0] = verb;
        } else {
            size_t i = 0;
            while (1) {
                if (i >= list->size) {
                    list->size = i + 1;
                    lame_realloc(&list->list, list->size * sizeof(struct Verb*));
                    list->list[i] = verb;
                    break;
                }
                if (list->list[i] == NULL) {
                    list->list[i] = verb;
                    break;
                }
                ++i;
            }
        }

        INFO("Assigned verb %s to mouse button %d", verb->name, mouse_button);
    }
}

void assign_verb_to_gamepad_button(struct Verb* verb, SDL_GamepadButton gamepad_button) {
    if (gamepad_button == verb->gamepad_button)
        return;

    if (verb->gamepad_button != NO_GAMEPAD_BUTTON) {
        const struct VerbList* list = &gamepad_button_map[verb->gamepad_button];
        if (list->list != NULL)
            for (size_t i = 0; i < list->size; i++)
                if (list->list[i] == verb) {
                    list->list[i] = NULL;
                    break;
                }

        INFO("Unassigned verb %s from gamepad button %d", verb->name, verb->gamepad_button);
    }

    verb->gamepad_button = gamepad_button;
    if (gamepad_button != NO_GAMEPAD_BUTTON) {
        struct VerbList* list = &gamepad_button_map[gamepad_button];
        if (list->list == NULL) {
            list->list = lame_alloc(sizeof(struct Verb*));
            list->size = 1;
            list->list[0] = verb;
        } else {
            size_t i = 0;
            while (1) {
                if (i >= list->size) {
                    list->size = i + 1;
                    lame_realloc(&list->list, list->size * sizeof(struct Verb*));
                    list->list[i] = verb;
                    break;
                }
                if (list->list[i] == NULL) {
                    list->list[i] = verb;
                    break;
                }
                ++i;
            }
        }

        INFO("Assigned verb %s to gamepad button %d", verb->name, gamepad_button);
    }
}

void assign_verb_to_gamepad_axis(struct Verb* verb, SDL_GamepadAxis gamepad_axis) {
    if (gamepad_axis == verb->gamepad_axis)
        return;

    if (verb->gamepad_axis != NO_GAMEPAD_AXIS) {
        const struct VerbList* list = &gamepad_axis_map[verb->gamepad_axis];
        if (list->list != NULL)
            for (size_t i = 0; i < list->size; i++)
                if (list->list[i] == verb) {
                    list->list[i] = NULL;
                    break;
                }

        INFO("Unassigned verb %s from gamepad axis %d", verb->name, verb->gamepad_axis);
    }

    verb->gamepad_axis = gamepad_axis;
    if (gamepad_axis != NO_GAMEPAD_AXIS) {
        struct VerbList* list = &gamepad_axis_map[gamepad_axis];
        if (list->list == NULL) {
            list->list = lame_alloc(sizeof(struct Verb*));
            list->size = 1;
            list->list[0] = verb;
        } else {
            size_t i = 0;
            while (1) {
                if (i >= list->size) {
                    list->size = i + 1;
                    lame_realloc(&list->list, list->size * sizeof(struct Verb*));
                    list->list[i] = verb;
                    break;
                }
                if (list->list[i] == NULL) {
                    list->list[i] = verb;
                    break;
                }
                ++i;
            }
        }

        INFO("Assigned verb %s to gamepad axis %d", verb->name, gamepad_axis);
    }
}

struct Verb* get_verb(enum Verbs verb) {
    return &verbs[verb];
}

struct Verb* find_verb(const char* name) {
    if (!SDL_HasProperty(verb_map, name))
        return NULL;
    const enum Verbs verb = (enum Verbs)SDL_GetNumberProperty(verb_map, name, VERB_SIZE);
    return verb >= VERB_SIZE ? NULL : &verbs[verb];
}

void handle_keyboard(SDL_KeyboardDeviceEvent* event) {}

void handle_key(SDL_KeyboardEvent* event) {
    const struct VerbList* list = &key_map[event->scancode];
    if (list->list == NULL)
        return;

    if (event->type == SDL_EVENT_KEY_DOWN)
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL) {
                verb->held = true;
                verb->value = VERB_VALUE_MAX;
            }
        }
    else
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL)
                verb->held = false;
        }
}

void handle_mouse(SDL_MouseDeviceEvent* event) {}

void handle_mouse_button(SDL_MouseButtonEvent* event) {
    enum MouseButtons mouse_button;
    switch (event->button) {
        default:
            mouse_button = MOUSE_NONE;
            break;
        case SDL_BUTTON_LEFT:
            mouse_button = MOUSE_LEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            mouse_button = MOUSE_MIDDLE;
            break;
        case SDL_BUTTON_RIGHT:
            mouse_button = MOUSE_RIGHT;
            break;
        case SDL_BUTTON_X1:
            mouse_button = MOUSE_X1;
            break;
        case SDL_BUTTON_X2:
            mouse_button = MOUSE_X2;
            break;
    }

    const struct VerbList* list = &mouse_button_map[mouse_button];
    if (list->list == NULL)
        return;

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL) {
                verb->held = true;
                verb->value = VERB_VALUE_MAX;
            }
        }
    else
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL)
                verb->held = false;
        }
}

void handle_mouse_wheel(SDL_MouseWheelEvent* event) {
    if (event->direction == SDL_MOUSEWHEEL_FLIPPED) {
        event->integer_x *= -1;
        event->integer_y *= -1;
    }
    const enum MouseButtons mouse_button =
        event->integer_y == 0
            ? (event->integer_x == 0 ? MOUSE_NONE : (event->integer_x > 0 ? MOUSE_WHEEL_LEFT : MOUSE_WHEEL_RIGHT))
            : (event->integer_y > 0 ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN);

    const struct VerbList* list = &mouse_button_map[mouse_button];
    if (list->list == NULL)
        return;

    for (size_t i = 0; i < list->size; i++) {
        struct Verb* verb = list->list[i];
        if (verb != NULL)
            verb->value = VERB_VALUE_MAX;
    }
}

void handle_gamepad(SDL_GamepadDeviceEvent* event) {
    // MEMORY LEAK: Windows leaks ~148 bytes when a gamepad is available.
    //              https://github.com/libsdl-org/SDL/issues/10621#issuecomment-2325450912
    // Pattern: <No HID devices f> 4E 6F 20 48 49 44 20 64 65 76 69 63 65 73 20 66
    if (event->type == SDL_EVENT_GAMEPAD_ADDED)
        SDL_OpenGamepad(event->which);
    else if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
        SDL_CloseGamepad(SDL_GetGamepadFromID(event->which));
}

void handle_gamepad_button(SDL_GamepadButtonEvent* event) {
    const struct VerbList* list = &gamepad_button_map[event->button];
    if (list->list == NULL)
        return;

    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL) {
                verb->held = true;
                verb->value = VERB_VALUE_MAX;
            }
        }
    else
        for (size_t i = 0; i < list->size; i++) {
            struct Verb* verb = list->list[i];
            if (verb != NULL)
                verb->held = false;
        }
}

void handle_gamepad_axis(SDL_GamepadAxisEvent* event) {
    const struct VerbList* list = &gamepad_axis_map[event->axis];
    if (list->list == NULL)
        return;

    for (size_t i = 0; i < list->size; i++) {
        struct Verb* verb = list->list[i];
        if (verb != NULL) {
            Sint16 value = verb->axis >= 0 ? SDL_max(event->value, 0) : -SDL_clamp(event->value, VERB_VALUE_MIN, 0);
            if (value < 8) {
                verb->held = false;
            } else {
                verb->held = true;
                verb->value = value;
            }
        }
    }
}

int16_t input_value(enum Verbs verb, size_t slot) {
    return verbs[verb].value;
}
