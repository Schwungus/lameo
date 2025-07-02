#pragma once

#include "L_memory.h"
#include "L_script.h" // IWYU pragma: keep

#define UI_Z 50

typedef HandleID UIID;

enum UIFlags { // These flags only apply while the target UI is active.
    UIF_NONE = 0,

    UIF_BLOCKING = 1 << 0,    // Freeze world
    UIF_BLOCK_INPUT = 1 << 1, // Block player input
    UIF_DRAW_SCREEN = 1 << 2, // Draw cameras

    UIF_DEFAULT = UIF_BLOCKING | UIF_DRAW_SCREEN,
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

    struct UI *parent, *child;
    int table;
    enum UIFlags flags;
};

void ui_init();
void ui_teardown();

int define_ui(lua_State*);

struct UI* create_ui(struct UI*, const char*);
void destroy_ui(struct UI*);

struct UI* hid_to_ui(UIID);
bool ui_is_ancestor(struct UI*, const char*);
struct UI* get_ui_root();
struct UI* get_ui_top();
