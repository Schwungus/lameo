#pragma once

#include "mem.h"
#include "script.h" // IWYU pragma: keep

#define UI_NAME_MAX 128

typedef HandleID UIID;

struct UIType {
    char name[UI_NAME_MAX];
    struct UIType* parent;
};

struct UI {
    UIID hid;
    struct UIType* type;

    struct UI *parent, *child;
};

void ui_init();
void ui_teardown();

int define_ui(lua_State*);
struct UIType* get_ui_type(const char*);

struct UI* create_ui(const char*, struct UI*);
void destroy_ui(struct UI*);

bool ui_is_ancestor(struct UI*, const char*);
