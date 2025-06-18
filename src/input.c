#include "input.h"
#include "log.h"
#include "mem.h"

static struct Verb verbs[VERB_SIZE] = {0};
static struct VerbList key_map[KEY_COUNT] = {0};
static struct VerbList mouse_button_map[MOUSE_BUTTON_COUNT] = {0};
static struct VerbList gamepad_button_map[GAMEPAD_BUTTON_COUNT] = {0};

void input_init() {
    define_verb(VERB_UP, "up", SDL_SCANCODE_W, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, -1);
    define_verb(VERB_LEFT, "left", SDL_SCANCODE_A, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, -1);
    define_verb(VERB_DOWN, "down", SDL_SCANCODE_S, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, 1);
    define_verb(VERB_RIGHT, "right", SDL_SCANCODE_D, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, 1);
    define_verb(VERB_WALK, "walk", SDL_SCANCODE_LCTRL, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, 0);

    define_verb(VERB_JUMP, "jump", SDL_SCANCODE_SPACE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_SOUTH, 0);
    define_verb(VERB_INTERACT, "interact", SDL_SCANCODE_E, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_EAST, 0);
    define_verb(VERB_ATTACK, "attack", SDL_SCANCODE_PERIOD, SDL_BUTTON_LEFT, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0);

    define_verb(
        VERB_INVENTORY1, "inventory1", SDL_SCANCODE_LSHIFT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, -1
    );
    define_verb(VERB_INVENTORY2, "inventory2", SDL_SCANCODE_1, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1);
    define_verb(VERB_INVENTORY3, "inventory3", SDL_SCANCODE_F, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 1);
    define_verb(VERB_INVENTORY4, "inventory4", SDL_SCANCODE_2, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 1);

    define_verb(VERB_AIM, "aim", SDL_SCANCODE_COMMA, SDL_BUTTON_RIGHT, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0);
    define_verb(VERB_AIM_UP, "aim_up", SDL_SCANCODE_UP, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_RIGHT_STICK, -1);
    define_verb(VERB_AIM_LEFT, "aim_left", SDL_SCANCODE_LEFT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_RIGHT_STICK, -1);
    define_verb(VERB_AIM_DOWN, "aim_down", SDL_SCANCODE_DOWN, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_RIGHT_STICK, 1);
    define_verb(VERB_AIM_RIGHT, "aim_right", SDL_SCANCODE_RIGHT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_RIGHT_STICK, 1);

    define_verb(VERB_UI_UP, "ui_up", SDL_SCANCODE_UP, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, -1);
    define_verb(VERB_UI_LEFT, "ui_left", SDL_SCANCODE_LEFT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, -1);
    define_verb(VERB_UI_DOWN, "ui_down", SDL_SCANCODE_DOWN, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, 1);
    define_verb(VERB_UI_RIGHT, "ui_right", SDL_SCANCODE_RIGHT, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_LEFT_STICK, 1);

    define_verb(VERB_UI_ENTER, "ui_enter", SDL_SCANCODE_RETURN, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_SOUTH, 0);
    define_verb(VERB_UI_CLICK, "ui_click", NO_KEY, SDL_BUTTON_LEFT, SDL_GAMEPAD_BUTTON_INVALID, 0);
    define_verb(VERB_UI_BACK, "ui_back", SDL_SCANCODE_ESCAPE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_START, 0);

    define_verb(VERB_PAUSE, "pause", SDL_SCANCODE_ESCAPE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_START, 0);
    define_verb(VERB_LEAVE, "leave", SDL_SCANCODE_BACKSPACE, NO_MOUSE_BUTTON, SDL_GAMEPAD_BUTTON_BACK, 0);

    define_verb(VERB_DEBUG_FPS, "debug_fps", SDL_SCANCODE_F1, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, 0);
    define_verb(VERB_DEBUG_CONSOLE, "debug_console", SDL_SCANCODE_BACKSLASH, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, 0);
    define_verb(
        VERB_DEBUG_CONSOLE_SUBMIT, "debug_console_submit", SDL_SCANCODE_RETURN, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, 0
    );
    define_verb(
        VERB_DEBUG_CONSOLE_PREVIOUS, "debug_console_previous", SDL_SCANCODE_UP, NO_MOUSE_BUTTON, NO_GAMEPAD_BUTTON, 0
    );

    INFO("Opened");
}

void input_update() {
    for (size_t i = 0; i < VERB_SIZE; i++)
        if (!verbs[i].held)
            verbs[i].value = 0;
}

void input_teardown() {
    size_t i;
    for (i = 0; i < KEY_COUNT; i++)
        if (key_map[i].list != NULL)
            lame_free(&key_map[i].list);
    for (i = 0; i < MOUSE_BUTTON_COUNT; i++)
        if (mouse_button_map[i].list != NULL)
            lame_free(&mouse_button_map[i].list);
    for (i = 0; i < GAMEPAD_BUTTON_COUNT; i++)
        if (gamepad_button_map[i].list != NULL)
            lame_free(&gamepad_button_map[i].list);

    INFO("Closed");
}

void define_verb(
    enum Verbs verb, const char* name, SDL_Scancode key, uint8_t mouse_button, SDL_GamepadButton gamepad_button,
    int8_t gamepad_axis
) {
    SDL_strlcpy(verbs[verb].name, name, VERB_NAME_MAX);

    verbs[verb].default_key = key;
    assign_verb_to_key(&verbs[verb], key);

    verbs[verb].default_mouse_button = mouse_button;
    assign_verb_to_mouse_button(&verbs[verb], mouse_button);

    verbs[verb].default_gamepad_button = verbs[verb].gamepad_button = gamepad_button;
    verbs[verb].default_gamepad_axis = verbs[verb].gamepad_axis = gamepad_axis;
}

void assign_verb_to_key(struct Verb* verb, SDL_Scancode key) {
    if (key == verb->key)
        return;

    if (verb->key != NO_KEY) {
        struct VerbList* list = &key_map[verb->key];
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

void assign_verb_to_mouse_button(struct Verb* verb, uint8_t mouse_button) {
    if (mouse_button == verb->mouse_button)
        return;

    if (verb->mouse_button != NO_MOUSE_BUTTON) {
        struct VerbList* list = &mouse_button_map[verb->mouse_button];
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

extern struct Verb* get_verb(enum Verbs verb) {
    return &verbs[verb];
}

void handle_keyboard(SDL_KeyboardDeviceEvent* event) {}

void handle_key(SDL_KeyboardEvent* event) {
    struct VerbList* list = &key_map[event->scancode];
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
    struct VerbList* list = &mouse_button_map[event->button];
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

extern int16_t input_value(enum Verbs verb, size_t slot) {
    return verbs[verb].value;
}
