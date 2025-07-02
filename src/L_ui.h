#pragma once

#include "L_memory.h"
#include "L_script.h" // IWYU pragma: keep

#define UI_NAME_MAX 128

typedef HandleID UIID;

struct UIType {
    const char* name;
    struct UIType* parent;
};

struct UI {
    UIID hid;
    struct UIType* type;

    struct UI *parent, *child;
    int table;
};

void ui_init();
void ui_teardown();

int define_ui(lua_State*);
struct UIType* get_ui_type(const char*);

struct UI* create_ui(struct UI*, const char*);
void destroy_ui(struct UI*);

bool ui_is_ancestor(struct UI*, const char*);
struct UI* get_ui_root();
struct UI* get_ui_top();
