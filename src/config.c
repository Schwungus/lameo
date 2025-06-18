#include "config.h"
#include "asset.h"
#include "input.h"
#include "log.h"
#include "video.h"

static const char *config_path = NULL, *controls_path = NULL;
static SDL_PropertiesID cvars = 0, default_cvars = 0;

void config_init(const char* confpath, const char* contpath) {
    if ((cvars = SDL_CreateProperties()) == 0)
        FATAL("CVars fail: %s", SDL_GetError());
    if ((default_cvars = SDL_CreateProperties()) == 0)
        FATAL("Default CVars fail: %s", SDL_GetError());

    SDL_SetStringProperty(default_cvars, "data_path", "data");

    SDL_SetNumberProperty(default_cvars, "vid_width", 0);
    SDL_SetNumberProperty(default_cvars, "vid_height", 0);
    SDL_SetNumberProperty(default_cvars, "vid_fullscreen", FSM_WINDOWED);
    SDL_SetBooleanProperty(default_cvars, "vid_vsync", false);
    SDL_SetNumberProperty(default_cvars, "vid_maxfps", 60);

    if (confpath != NULL) {
        config_path = confpath;
        INFO("Using config file \"%s\"", config_path);
    }
    if (contpath != NULL) {
        controls_path = contpath;
        INFO("Using controls file \"%s\"", controls_path);
    }
    load_config();
    apply_cvar(NULL);

    INFO("Opened");
}

void config_teardown() {
    SDL_DestroyProperties(cvars);
    SDL_DestroyProperties(default_cvars);

    INFO("Closed");
}

extern bool get_bool_cvar(const char* name) {
    return SDL_GetBooleanProperty(cvars, name, SDL_GetBooleanProperty(default_cvars, name, false));
}

extern Sint64 get_int_cvar(const char* name) {
    return SDL_GetNumberProperty(cvars, name, SDL_GetNumberProperty(default_cvars, name, 0));
}

extern float get_float_cvar(const char* name) {
    return SDL_GetFloatProperty(cvars, name, SDL_GetFloatProperty(default_cvars, name, 0));
}

extern const char* get_string_cvar(const char* name) {
    return SDL_GetStringProperty(cvars, name, SDL_GetStringProperty(default_cvars, name, ""));
}

void set_bool_cvar(const char* name, bool value) {
    if (SDL_GetPropertyType(default_cvars, name) == SDL_PROPERTY_TYPE_BOOLEAN)
        SDL_SetBooleanProperty(cvars, name, value);
    else
        WTF("Unknown or non-boolean CVar \"%s\"", name);
}

void set_int_cvar(const char* name, Sint64 value) {
    if (SDL_GetPropertyType(default_cvars, name) == SDL_PROPERTY_TYPE_NUMBER)
        SDL_SetNumberProperty(cvars, name, value);
    else
        WTF("Unknown or non-integer CVar \"%s\"", name);
}

void set_float_cvar(const char* name, float value) {
    if (SDL_GetPropertyType(default_cvars, name) == SDL_PROPERTY_TYPE_FLOAT)
        SDL_SetFloatProperty(cvars, name, value);
    else
        WTF("Unknown or non-float CVar \"%s\"", name);
}

void set_numeric_cvar(const char* name, double value) {
    switch (SDL_GetPropertyType(default_cvars, name)) {
        case SDL_PROPERTY_TYPE_BOOLEAN:
            SDL_SetBooleanProperty(cvars, name, (bool)value);
            break;
        case SDL_PROPERTY_TYPE_NUMBER:
            SDL_SetNumberProperty(cvars, name, (Sint64)value);
            break;
        case SDL_PROPERTY_TYPE_FLOAT:
            SDL_SetFloatProperty(cvars, name, (float)value);
            break;
        default:
            WTF("Unknown or non-numeric CVar \"%s\"", name);
            break;
    }
}

void set_string_cvar(const char* name, const char* value) {
    if (SDL_GetPropertyType(default_cvars, name) == SDL_PROPERTY_TYPE_STRING)
        SDL_SetStringProperty(cvars, name, value);
    else
        WTF("Unknown or non-string CVar \"%s\"", name);
}

static void iterate_reset_cvar(void* userdata, SDL_PropertiesID props, const char* name) {
    bool* result = (bool*)userdata;
    *result = *result && reset_cvar(name);
}

bool reset_cvar(const char* name) {
    if (name == NULL) {
        bool result;
        SDL_EnumerateProperties(default_cvars, iterate_reset_cvar, &result);
        return result;
    }

    switch (SDL_GetPropertyType(default_cvars, name)) {
        case SDL_PROPERTY_TYPE_BOOLEAN:
            return SDL_SetBooleanProperty(cvars, name, SDL_GetBooleanProperty(default_cvars, name, false));
        case SDL_PROPERTY_TYPE_NUMBER:
            return SDL_SetNumberProperty(cvars, name, SDL_GetNumberProperty(default_cvars, name, 0));
        case SDL_PROPERTY_TYPE_FLOAT:
            return SDL_SetFloatProperty(cvars, name, SDL_GetFloatProperty(default_cvars, name, 0));
        case SDL_PROPERTY_TYPE_STRING:
            return SDL_SetStringProperty(cvars, name, SDL_GetStringProperty(default_cvars, name, ""));
    }
    return false;
}

void apply_cvar(const char* name) {
    if (name == NULL || (SDL_strcmp(name, "vid_width") == 0 || SDL_strcmp(name, "vid_height") == 0 ||
                         SDL_strcmp(name, "vid_fullscreen") == 0 || SDL_strcmp(name, "vid_vsync") == 0))
        set_display(
            get_int_cvar("vid_width"), get_int_cvar("vid_height"), get_int_cvar("vid_fullscreen"),
            get_bool_cvar("vid_vsync")
        );

    if (name == NULL || SDL_strcmp(name, "vid_maxfps") == 0) {
        Sint64 vid_maxfps = get_int_cvar("vid_maxfps");
        set_framerate((int16_t)SDL_max(vid_maxfps, 0));
    }
}

static void iterate_save_config(void* userdata, SDL_PropertiesID props, const char* name) {
    yyjson_mut_doc* json = (yyjson_mut_doc*)userdata;

    yyjson_mut_val* key = yyjson_mut_strcpy(json, name);
    yyjson_mut_val* value;
    switch (SDL_GetPropertyType(props, name)) {
        default:
            value = yyjson_mut_null(json);
            break;
        case SDL_PROPERTY_TYPE_BOOLEAN:
            value = yyjson_mut_bool(json, get_bool_cvar(name));
            break;
        case SDL_PROPERTY_TYPE_NUMBER:
            value = yyjson_mut_sint(json, get_int_cvar(name));
            break;
        case SDL_PROPERTY_TYPE_FLOAT:
            value = yyjson_mut_float(json, get_float_cvar(name));
            break;
        case SDL_PROPERTY_TYPE_STRING:
            value = yyjson_mut_strcpy(json, get_string_cvar(name));
            break;
    }

    yyjson_mut_obj_add(yyjson_mut_doc_get_root(json), key, value);
}

void save_config() {
    if (config_path == NULL) {
        yyjson_mut_doc* json = yyjson_mut_doc_new(NULL);
        yyjson_mut_val* root = yyjson_mut_obj(json);

        yyjson_mut_doc_set_root(json, root);
        SDL_EnumerateProperties(cvars, iterate_save_config, (void*)json);

        yyjson_write_err error;
        if (yyjson_mut_write_file("config.json", json, JSON_WRITE_FLAGS, NULL, &error))
            INFO("Saved config");
        else
            WTF("Config save fail: %s", error.msg);
        yyjson_mut_doc_free(json);
    } else {
        WARN("Cannot overwrite custom config file");
    }

    if (controls_path == NULL) {
        yyjson_mut_doc* json = yyjson_mut_doc_new(NULL);
        yyjson_mut_val* root = yyjson_mut_obj(json);

        yyjson_mut_doc_set_root(json, root);
        for (size_t i = 0; i < VERB_SIZE; i++) {
            const struct Verb* verb = get_verb(i);
            yyjson_mut_val* key = yyjson_mut_strcpy(json, verb->name);
            yyjson_mut_val* value = yyjson_mut_arr(json);
            yyjson_mut_arr_add_int(json, value, verb->key);
            yyjson_mut_arr_add_int(json, value, verb->mouse_button);
            yyjson_mut_arr_add_int(json, value, verb->gamepad_button);
            yyjson_mut_arr_add_int(json, value, verb->gamepad_axis);
            yyjson_mut_obj_add(root, key, value);
        }

        yyjson_write_err error;
        if (yyjson_mut_write_file("controls.json", json, JSON_WRITE_FLAGS, NULL, &error))
            INFO("Saved controls");
        else
            WTF("Controls save fail: %s", error.msg);
        yyjson_mut_doc_free(json);
    } else {
        WARN("Cannot overwrite custom controls file");
    }
}

void load_config() {
    // Config
    const char* path = config_path == NULL ? "config.json" : config_path;
    yyjson_read_err error;
    yyjson_doc* json = yyjson_read_file(path, JSON_FLAGS, NULL, &error);
    if (json == NULL) {
        WTF("Failed to open config \"%s\": %s", path, error.msg);
    } else {
        yyjson_val* root = yyjson_doc_get_root(json);
        if (!yyjson_is_obj(root)) {
            WTF("Expected config \"%s\" as object, got %s", path, yyjson_get_type_desc(root));
            yyjson_doc_free(json);
        } else {
            size_t i, n;
            yyjson_val *key, *value;
            yyjson_obj_foreach(root, i, n, key, value) {
                const char* cname = yyjson_get_str(key);
                switch (yyjson_get_type(value)) {
                    case YYJSON_TYPE_BOOL:
                        set_bool_cvar(cname, yyjson_get_bool(value));
                        break;
                    case YYJSON_TYPE_NUM:
                        set_numeric_cvar(cname, yyjson_get_num(value));
                        break;
                    case YYJSON_TYPE_STR:
                        set_string_cvar(cname, yyjson_get_str(value));
                        break;
                    default:
                        WTF("Invalid type \"%s\" for CVar \"%s\"", yyjson_get_type_desc(value), cname);
                        break;
                }
            }

            yyjson_doc_free(json);

            INFO("Loaded config");
        }
    }

    // Controls
    path = controls_path == NULL ? "controls.json" : controls_path;
    json = yyjson_read_file(path, JSON_FLAGS, NULL, &error);
    if (json == NULL) {
        WTF("Failed to open controls \"%s\": %s", path, error.msg);
    } else {
        yyjson_val* root = yyjson_doc_get_root(json);
        if (!yyjson_is_obj(root)) {
            WTF("Expected controls \"%s\" as object, got %s", path, yyjson_get_type_desc(root));
            yyjson_doc_free(json);
        } else {
            size_t i, n;
            yyjson_val *key, *value;
            yyjson_obj_foreach(root, i, n, key, value) {
                const char* vname = yyjson_get_str(key);

                if (!yyjson_is_arr(value)) {
                    WTF("Expected verb \"%s\" as array, got %s", vname, yyjson_get_type_desc(root));
                    continue;
                }
                if (yyjson_arr_size(value) < 4) {
                    WTF("Verb \"%s\" array size less than 4", vname);
                    continue;
                }

                struct Verb* verb = find_verb(vname);
                if (verb == NULL) {
                    WARN("Unknown verb \"%s\"", vname);
                    continue;
                }
                assign_verb_to_key(verb, (SDL_Scancode)yyjson_get_uint(yyjson_arr_get(value, 0)));
                assign_verb_to_mouse_button(verb, (enum MouseButtons)yyjson_get_uint(yyjson_arr_get(value, 1)));
                assign_verb_to_gamepad_button(verb, (SDL_GamepadButton)yyjson_get_sint(yyjson_arr_get(value, 2)));
                assign_verb_to_gamepad_axis(verb, (SDL_GamepadAxis)yyjson_get_sint(yyjson_arr_get(value, 3)));
            }

            yyjson_doc_free(json);

            INFO("Loaded controls");
        }
    }
}
