#include "L_actor.h"
#include "L_log.h"
#include "L_memory.h"

static struct HashMap* actor_types = NULL;

void actor_init() {
    actor_types = create_hash_map();

    INFO("Opened");
}

void actor_teardown() {
    destroy_hash_map(actor_types, true);

    INFO("Closed");
}
