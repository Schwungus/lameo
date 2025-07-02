#include "L_ui.h"
#include "L_log.h"
#include "L_memory.h"

static struct HashMap* ui_types = NULL;
static struct UI *ui_root = NULL, *ui_top = NULL;
static struct Fixture* ui_handles = NULL;

void ui_init() {
    ui_types = create_hash_map();
    ui_handles = create_fixture();

    INFO("Opened");
}

void ui_teardown() {
    if (ui_root != NULL)
        destroy_ui(ui_root);

    for (size_t i = 0; ui_types->count > 0 && i < ui_types->capacity; i++) {
        struct KeyValuePair* kvp = &ui_types->items[i];
        if (kvp->key == NULL)
            continue;

        struct UIType* type = kvp->value;
        if (type != NULL) {
            lame_free(&type->name);
            unreference(&type->load);
            unreference(&type->create);
            unreference(&type->cleanup);
            unreference(&type->tick);
            unreference(&type->draw);
            lame_free(&kvp->value);
        }

        lame_free(&kvp->key);
        ui_types->count--;
    }

    destroy_hash_map(ui_types, false);
    CLOSE_POINTER(ui_handles, destroy_fixture);

    INFO("Closed");
}

int define_ui(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const char* parent_name = luaL_optstring(L, 2, NULL);
    luaL_checktype(L, 3, LUA_TTABLE);

    struct UIType* parent = NULL;
    if (parent_name != NULL) {
        if (SDL_strcmp(name, parent_name) == 0)
            FATAL("UI \"%s\" inheriting itself?", name);
        parent = from_hash_map(ui_types, parent_name);
        if (parent == NULL)
            FATAL("UI \"%s\" inheriting from unknown type \"%s\"", name, parent_name);
    }

    struct UIType* type = from_hash_map(ui_types, name);
    if (type == NULL) {
        type = lame_alloc(sizeof(struct UIType));
        type->name = SDL_strdup(name);
        to_hash_map(ui_types, name, type, true);
    } else {
        unreference(&type->load);
        unreference(&type->create);
        unreference(&type->cleanup);
        unreference(&type->tick);
        unreference(&type->draw);
    }

    type->parent = parent;

    type->load = function_ref(-1, "load");
    type->create = function_ref(-1, "create");
    type->cleanup = function_ref(-1, "cleanup");
    type->tick = function_ref(-1, "tick");
    type->draw = function_ref(-1, "draw");

    INFO("Defined UI \"%s\"", name);

    return 0;
}

struct UI* create_ui(struct UI* parent, const char* name) {
    struct UIType* type = from_hash_map(ui_types, name);
    if (type == NULL) {
        WTF("Unknown UI type \"%s\"", name);
        return NULL;
    }

    struct UI* ui = lame_alloc(sizeof(struct UI));
    UIID hid = create_handle(ui_handles, ui);
    ui->hid = hid;
    ui->type = type;

    if (parent == NULL) {
        if (ui_root != NULL)
            destroy_ui(ui_root);
        ui_root = ui;
    } else {
        if (parent->child != NULL)
            destroy_ui(parent->child);
        parent->child = ui;
    }
    ui->parent = parent;
    ui->child = NULL;
    ui_top = ui;

    ui->table = create_table_ref();
    if (type->create != LUA_NOREF)
        execute_ref_in(type->create, ui->hid, name);

    return hid_to_ui(hid) != NULL ? ui : NULL;
}

void destroy_ui(struct UI* ui) {
    if (ui_root == ui)
        ui_root = NULL;
    if (ui_top == ui)
        ui_top = ui->parent;

    if (ui->parent != NULL)
        ui->parent->child = NULL;
    if (ui->child != NULL)
        destroy_ui(ui->child);

    unreference(&ui->table);

    destroy_handle(ui_handles, ui->hid);
    lame_free(&ui);
}

struct UI* hid_to_ui(UIID hid) {
    return (struct UI*)hid_to_pointer(ui_handles, hid);
}

struct UI* get_ui_root() {
    return ui_root;
}

struct UI* get_ui_top() {
    return ui_top;
}
