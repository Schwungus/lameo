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

    destroy_hash_map(ui_types, true);
    CLOSE_POINTER(ui_handles, destroy_fixture);

    INFO("Closed");
}

struct UI* create_ui(struct UI* parent, const char* name) {
    struct UIType* type = from_hash_map(ui_types, name);
    if (type == NULL) {
        WTF("Unknown UI type \"%s\"", name);
        return NULL;
    }

    struct UI* ui = lame_alloc(sizeof(struct UI));
    ui->hid = create_handle(ui_handles, ui);
    ui->type = type;

    if (parent == NULL) {
        if (ui_root != NULL)
            destroy_ui(ui_root);
        ui_root = ui;
    } else {
        if (parent->child != NULL)
            destroy_ui(parent->child);
        parent->child = ui;
        ui->parent = parent;
    }
    ui->child = NULL;

    ui->table = create_table_ref();

    ui_top = ui;
    return ui;
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

struct UI* get_ui_root() {
    return ui_root;
}

struct UI* get_ui_top() {
    return ui_top;
}
