#include "L_level.h"
#include "L_file.h"
#include "L_internal.h"
#include "L_log.h"
#include "L_mod.h"

static struct Level* level = NULL;

void unload_level() {
    if (level == NULL)
        return;

    for (size_t i = 0; level->rooms->count > 0 && i < level->rooms->capacity; i++) {
        struct IKeyValuePair* kvp = &level->rooms->items[i];
        if (kvp->state != IKVP_OCCUPIED)
            continue;

        struct Room* room = kvp->value;
        if (room != NULL)
            destroy_room(room);
        kvp->value = NULL;
        kvp->state = IKVP_DELETED;

        level->rooms->count--;
    }
    destroy_int_map(level->rooms, true);

    lame_free(&(level->name));
    lame_free(&(level->title));
    lame_free(&level);
}

void load_level(const char* name, uint32_t room, uint16_t tag) {
    if (level != NULL) {
        WTF("Unload the level first...");
        return;
    }

    char level_file_helper[FILE_PATH_MAX];
    SDL_snprintf(level_file_helper, sizeof(level_file_helper), "levels/%s.json", name);
    yyjson_doc* json = load_json(get_mod_file(level_file_helper, NULL));
    if (json == NULL)
        FATAL("Invalid level \"%s\"", name);
    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root))
        FATAL("Expected level \"%s\" root as object, got %s", name, yyjson_get_type_desc(root));

    level = lame_alloc_clean(sizeof(struct Level));
    level->name = SDL_strdup(name);

    // Properties
    yyjson_val* value = yyjson_obj_get(root, "title");
    level->title = SDL_strdup(yyjson_is_str(value) ? yyjson_get_str(value) : name);

    // Rooms
    level->rooms = create_int_map();
    value = yyjson_obj_get(root, "rooms");
    if (yyjson_is_arr(value)) {
        yyjson_val* roomdef;
        size_t i, n;
        yyjson_arr_foreach(value, i, n, roomdef) {
            if (!yyjson_is_obj(roomdef))
                FATAL("Expected level \"%s\" room %u as object, got %s", name, i, yyjson_get_type_desc(root));
            yyjson_val* roomval = yyjson_obj_get(roomdef, "id");
            if (!yyjson_is_uint(roomval))
                FATAL("Expected level \"%s\" room %u ID as uint, got %s", name, i, yyjson_get_type_desc(roomval));

            // Allocate room at this point
            struct Room* room = lame_alloc_clean(sizeof(struct Room));
            room->level = level;
            room->id = (uint32_t)yyjson_get_uint(roomval);
            if (!to_int_map(level->rooms, room->id, room, false))
                FATAL(
                    "Room %u tried to reoccupy ID %u in level \"%s\" (make sure it's free and within uint32 limits)", i,
                    room->id, name
                );

            room->bump.size[0] = room->bump.size[1] = 1;

            // Properties
            roomval = yyjson_obj_get(roomdef, "model");
            if (yyjson_is_str(roomval)) {
                struct Model* model = fetch_model(yyjson_get_str(roomval));
                if (model != NULL)
                    room->model = create_model_instance(model);
            }

            // Actors
            roomval = yyjson_obj_get(roomdef, "actors");
            if (yyjson_is_arr(roomval)) {
                yyjson_val* actdef;
                size_t j, o;
                yyjson_arr_foreach(roomval, j, o, actdef) {
                    if (!yyjson_is_obj(actdef))
                        FATAL(
                            "Expected level \"%s\" room ID %u actor %u as object, got %s", name, room->id, j,
                            yyjson_get_type_desc(actdef)
                        );
                    yyjson_val* actval = yyjson_obj_get(actdef, "type");
                    if (!yyjson_is_str(actval))
                        FATAL(
                            "Expected level \"%s\" room ID %u actor %u type as string, got %s", name, room->id, j,
                            yyjson_get_type_desc(actdef)
                        );
                    const char* type = yyjson_get_str(actval);
                    if (!load_actor(type))
                        continue;

                    struct RoomActor* room_actor = lame_alloc_clean(sizeof(struct RoomActor));
                    room_actor->type = get_actor_type(type);

                    if (room->room_actors != NULL)
                        room->room_actors->next = room_actor;
                    room_actor->previous = room->room_actors;
                    room->room_actors = room_actor;

                    room_actor->pos[0] = (float)yyjson_get_real(yyjson_obj_get(actdef, "x"));
                    room_actor->pos[1] = (float)yyjson_get_real(yyjson_obj_get(actdef, "y"));
                    room_actor->pos[2] = (float)yyjson_get_real(yyjson_obj_get(actdef, "z"));
                    room_actor->angle[0] = (float)yyjson_get_real(yyjson_obj_get(actdef, "yaw"));
                    room_actor->angle[1] = (float)yyjson_get_real(yyjson_obj_get(actdef, "pitch"));
                    room_actor->angle[2] = (float)yyjson_get_real(yyjson_obj_get(actdef, "roll"));
                    room_actor->tag = (uint16_t)yyjson_get_uint(yyjson_obj_get(actdef, "tag"));

                    room_actor->flags = RAF_DEFAULT;
                    if (yyjson_get_bool(yyjson_obj_get(actdef, "persistent")))
                        room_actor->flags |= RAF_PERSISTENT;
                    if (yyjson_get_bool(yyjson_obj_get(actdef, "disposable")))
                        room_actor->flags |= RAF_DISPOSABLE;

                    update_bump_map(&room->bump, room_actor->pos);
                }
            } else {
                WTF("Expected level \"%s\" room ID %u actors as array, got %s", name, room->id,
                    yyjson_get_type_desc(root));
            }
        }
    } else {
        WTF("Expected level \"%s\" rooms as array, got %s", name, yyjson_get_type_desc(root));
    }

    // Assets

    yyjson_doc_free(json);
}

void go_to_level(const char* name, uint32_t room, uint16_t tag) {
    start_loading(name, room, tag);
}

struct Level* get_level() {
    return level;
}
