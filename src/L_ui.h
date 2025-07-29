#pragma once

#include "L_math.h" // IWYU pragma: keep
#include "L_memory.h"
#include "L_script.h" // IWYU pragma: keep

#define UI_Z -50

typedef HandleID UIID;

enum UIFlags { // These flags only apply while the target UI is active.
    UIF_NONE = 0,

    UIF_BLOCKING = 1 << 0,    // Freeze world
    UIF_BLOCK_INPUT = 1 << 1, // Block player input
    UIF_DRAW_SCREEN = 1 << 2, // Draw cameras

    UIF_DEFAULT = UIF_BLOCKING | UIF_DRAW_SCREEN,
};

enum UIButtons {
    UII_NONE = 0,

    UII_UP = 1 << 0,
    UII_LEFT = 1 << 1,
    UII_DOWN = 1 << 2,
    UII_RIGHT = 1 << 3,

    UII_ENTER = 1 << 4,
    UII_CLICK = 1 << 5,
    UII_BACK = 1 << 6,
};

struct UIInput {
    vec2 cursor;
    enum UIButtons buttons;
};

struct UIType {
    const char* name;
    struct UIType* parent;

    int load;
    int create;
    int cleanup;
    int tick;
    int draw;
};

struct UI {
    UIID hid;
    struct UIType* type;
    int userdata;

    struct UI *parent, *child;
    int table;
    enum UIFlags flags;
};

void ui_init();
void ui_teardown();

int define_ui(lua_State*);
bool load_ui(const char*);

struct UI* create_ui(struct UI*, const char*);
void destroy_ui(struct UI*);

struct UI* hid_to_ui(UIID);
bool ui_is_ancestor(struct UI*, const char*);
struct UI* get_ui_root();
struct UI* get_ui_top();
bool ui_blocking();

void update_ui_input();
const vec2* get_ui_cursor();
bool get_ui_buttons(enum UIButtons);
const vec2* get_last_ui_cursor();
bool get_last_ui_buttons(enum UIButtons);
