#include "L_ui.h"
#include "L_input.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_video.h"

static struct HashMap* ui_types = NULL;
static struct UI *ui_root = NULL, *ui_top = NULL;
static struct Fixture* ui_handles = NULL;

static struct UIInput ui_input = {0}, last_ui_input = {0};

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
        WARN("Redefining UI \"%s\"", name);
        if (type->parent == NULL || type->load != parent->load)
            unreference(&type->load);
        if (type->parent == NULL || type->create != parent->create)
            unreference(&type->create);
        if (type->parent == NULL || type->cleanup != parent->cleanup)
            unreference(&type->cleanup);
        if (type->parent == NULL || type->tick != parent->tick)
            unreference(&type->tick);
        if (type->parent == NULL || type->draw != parent->draw)
            unreference(&type->draw);
    }

    type->parent = parent;

    type->load = function_ref(-1, "load");
    type->create = function_ref(-1, "create");
    type->cleanup = function_ref(-1, "cleanup");
    type->tick = function_ref(-1, "tick");
    type->draw = function_ref(-1, "draw");

    if (parent != NULL) {
        if (type->load == LUA_NOREF)
            type->load = parent->load;
        if (type->create == LUA_NOREF)
            type->create = parent->create;
        if (type->cleanup == LUA_NOREF)
            type->cleanup = parent->cleanup;
        if (type->tick == LUA_NOREF)
            type->tick = parent->tick;
        if (type->draw == LUA_NOREF)
            type->draw = parent->draw;
    }

    SCRIPT_LOG(L, "Defined UI \"%s\"", name);
    return 0;
}

bool load_ui(const char* name) {
    struct UIType* type = from_hash_map(ui_types, name);
    if (type == NULL) {
        WTF("Unknown UI type \"%s\"", name);
        return false;
    }

    struct UIType* it = type;
    do {
        if (it->load != LUA_NOREF)
            execute_ref(it->load, it->name);
        INFO("Loaded UI \"%s\"", it->name);
        it = it->parent;
    } while (it != NULL);

    return true;
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
    ui->flags = UIF_DEFAULT;

    if (type->create != LUA_NOREF)
        execute_ref_in(type->create, ui->hid, name);
    return hid_to_ui(hid) != NULL ? ui : NULL;
}

void destroy_ui(struct UI* ui) {
    if (ui->type->cleanup != LUA_NOREF)
        execute_ref_in(ui->type->cleanup, ui->hid, ui->type->name);

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

bool ui_is_ancestor(struct UI* ui, const char* name) {
    struct UIType* it = ui->type;
    while (it != NULL) {
        if (SDL_strcmp(it->name, name) == 0)
            return true;
        it = it->parent;
    }

    return false;
}

struct UI* get_ui_root() {
    return ui_root;
}

struct UI* get_ui_top() {
    return ui_top;
}

void update_ui_input() {
    // Save last input
    glm_vec2_copy(ui_input.cursor, last_ui_input.cursor);
    last_ui_input.buttons = ui_input.buttons;

    // Cursor position normalized to UI space (i.e. 640x480)
    SDL_GetMouseState(&ui_input.cursor[0], &ui_input.cursor[1]);

    const struct Display* display = get_display();
    float scale, offset_x = 0, offset_y = 0;
    if (((float)display->width / (float)display->height) >
        ((float)DEFAULT_DISPLAY_WIDTH / (float)DEFAULT_DISPLAY_HEIGHT)) { // Pillarbox
        scale = (float)display->height / (float)DEFAULT_DISPLAY_HEIGHT;
        offset_x = ((float)display->width - (DEFAULT_DISPLAY_WIDTH * scale)) / 2.0f;
    } else { // Letterbox
        scale = (float)display->width / (float)DEFAULT_DISPLAY_WIDTH;
        offset_y = ((float)display->height - (DEFAULT_DISPLAY_HEIGHT * scale)) / 2.0f;
    }

    ui_input.cursor[0] = (ui_input.cursor[0] - offset_x) / scale;
    ui_input.cursor[1] = (ui_input.cursor[1] - offset_y) / scale;

    ui_input.buttons = 0;
    if (input_value(VERB_UI_UP, 0))
        ui_input.buttons |= UII_UP;
    if (input_value(VERB_UI_LEFT, 0))
        ui_input.buttons |= UII_LEFT;
    if (input_value(VERB_UI_DOWN, 0))
        ui_input.buttons |= UII_DOWN;
    if (input_value(VERB_UI_RIGHT, 0))
        ui_input.buttons |= UII_RIGHT;
    if (input_value(VERB_UI_ENTER, 0))
        ui_input.buttons |= UII_ENTER;
    if (input_value(VERB_UI_CLICK, 0))
        ui_input.buttons |= UII_CLICK;
    if (input_value(VERB_UI_BACK, 0))
        ui_input.buttons |= UII_BACK;
}

const vec2* get_ui_cursor() {
    return &ui_input.cursor;
}

bool get_ui_buttons(enum UIButtons buttons) {
    return (ui_input.buttons & buttons) == buttons;
}

const vec2* get_last_ui_cursor() {
    return &last_ui_input.cursor;
}

bool get_last_ui_buttons(enum UIButtons buttons) {
    return (last_ui_input.buttons & buttons) == buttons;
}
