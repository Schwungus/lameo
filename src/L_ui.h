#pragma once

#include "L_memory.h"
#include "L_script.h" // IWYU pragma: keep

#define UI_NAME_MAX 128

#define UI_HOOK_FUNCTION(funcname)                                                                                     \
    lua_getfield(L, -1, #funcname);                                                                                    \
    if (lua_isfunction(L, -1)) {                                                                                       \
        type->funcname = luaL_ref(L, LUA_REGISTRYINDEX);                                                               \
    } else {                                                                                                           \
        type->funcname = LUA_NOREF;                                                                                    \
        lua_pop(L, 1);                                                                                                 \
    }

typedef HandleID UIID;

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
