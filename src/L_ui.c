#include "L_ui.h"
#include "L_log.h"
#include "L_memory.h"

static struct UIType* ui_types = NULL;
static struct UI* uis = NULL;
static struct Fixture* ui_handles = NULL;

void ui_init() {
    ui_handles = create_fixture();

    INFO("Opened");
}

void ui_teardown() {
    CLOSE_POINTER(ui_handles, destroy_fixture);

    INFO("Closed");
}
