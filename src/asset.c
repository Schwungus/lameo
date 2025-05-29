#include <SDL3/SDL_stdinc.h>
#include <SDL3_image/SDL_image.h>
#include <yyjson.h>

#include "asset.h"
#include "audio.h"
#include "log.h"
#include "mod.h"

#define ASSET_PATH_MAX 256

static struct Shader* shaders = NULL;
static struct Fixture* shader_handles = NULL;

static struct Texture* textures = NULL;
static struct Fixture* texture_handles = NULL;

static struct Material* materials = NULL;
static struct Fixture* material_handles = NULL;

static struct Model* models = NULL;
static struct Fixture* model_handles = NULL;

static struct Font* fonts = NULL;
static struct Fixture* font_handles = NULL;

static struct Sound* sounds = NULL;
static struct Fixture* sound_handles = NULL;

static struct Track* music = NULL;
static struct Fixture* track_handles = NULL;

void asset_init() {
    shader_handles = create_fixture();
    texture_handles = create_fixture();
    material_handles = create_fixture();
    model_handles = create_fixture();
    font_handles = create_fixture();
    sound_handles = create_fixture();
    track_handles = create_fixture();

    video_init_render();

    INFO("Opened");
}

void asset_teardown() {
    clear_shaders(1);
    clear_textures(1);
    clear_sounds(1);
    clear_music(1);

    destroy_fixture(shader_handles);
    destroy_fixture(texture_handles);
    destroy_fixture(material_handles);
    destroy_fixture(model_handles);
    destroy_fixture(font_handles);
    destroy_fixture(sound_handles);
    destroy_fixture(track_handles);

    INFO("Closed");
}

static char path_helper[ASSET_PATH_MAX];

// Shaders
void load_shader(const char* name) {
    if (get_shader(name) != NULL)
        return;

    // Vertex shader
    SDL_snprintf(path_helper, sizeof(path_helper), "shaders/%s.vsh", name);
    const char* file = get_file(path_helper, NULL);
    if (file == NULL) {
        WARN("Vertex shader for \"%s\" not found", name);
        return;
    }

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);

    GLchar* code = (GLchar*)SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Failed to load vertex shader \"%s\": %s", name, SDL_GetError());
    glShaderSource(vertex, 1, &code, NULL);
    glCompileShader(vertex);

    GLint success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024); // Why not reuse the code string?
        glGetShaderInfoLog(vertex, 1024, NULL, code);
        FATAL("Failed to compile vertex shader \"%s\":\n%s", name, code);
    }
    lame_free(&code);

    // Fragment shader
    SDL_snprintf(path_helper, sizeof(path_helper), "shaders/%s.fsh", name);
    file = get_file(path_helper, NULL);
    if (file == NULL) {
        WARN("Fragment shader for \"%s\" not found", name);
        glDeleteShader(vertex);
        return;
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

    code = (GLchar*)SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Failed to load fragment shader for \"%s\": %s", name, SDL_GetError());
    glShaderSource(fragment, 1, &code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024);
        glGetShaderInfoLog(fragment, 1024, NULL, code);
        FATAL("Failed to compile fragment shader for \"%s\":\n%s", name, code);
    }
    // Hold on to "code", we still have to attach the shaders.
    // lame_free(&code);

    // Shader struct
    struct Shader* shader = lame_alloc(sizeof(struct Shader));

    // General
    shader->hid = (ShaderID)create_handle(shader_handles, shader);
    SDL_strlcpy(shader->name, name, sizeof(shader->name));
    shader->transient = 1; // No sense in unloading shaders
    if (shaders != NULL)
        shaders->next = shader;
    shader->previous = shaders;
    shader->next = NULL;
    shaders = shader;

    // Program
    shader->program = glCreateProgram();
    glAttachShader(shader->program, vertex);
    glAttachShader(shader->program, fragment);
    glBindAttribLocation(shader->program, VATT_POSITION, "i_position");
    glBindAttribLocation(shader->program, VATT_NORMAL, "i_normal");
    glBindAttribLocation(shader->program, VATT_COLOR, "i_color");
    glBindAttribLocation(shader->program, VATT_UV, "i_uv");
    glBindAttribLocation(shader->program, VATT_BONE_INDEX, "i_bone_index");
    glBindAttribLocation(shader->program, VATT_BONE_WEIGHT, "i_bone_weight");
    glLinkProgram(shader->program);

    glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024);
        glGetProgramInfoLog(shader->program, 1024, NULL, code);
        FATAL("Failed to link shader program for \"%s\":\n%s", name, code);
    }
    lame_free(&code); // Now we're good

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Uniforms
    if ((shader->uniforms = SDL_CreateProperties()) == 0)
        FATAL("Shader \"%s\" uniforms fail: %s", name, SDL_GetError());
    GLsizei unum, ulen;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &(GLint)unum);
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &(GLint)ulen);
    for (GLsizei i = 0; i < unum; i++) {
        GLchar uname[ASSET_NAME_MAX];
        glGetActiveUniform(shader->program, i, ulen, NULL, NULL, NULL, uname);
        if (!SDL_SetNumberProperty(shader->uniforms, uname, (Sint64)glGetUniformLocation(shader->program, uname)))
            FATAL("Shader \"%s\" uniform \"%s\" fail: %s", name, uname, SDL_GetError());
    }

    INFO("Loaded shader \"%s\" (%u)", name, shader->hid);
}

struct Shader* get_shader(const char* name) {
    for (struct Shader* shader = shaders; shader != NULL; shader = shader->previous)
        if (SDL_strcmp(shader->name, name) == 0)
            return shader;
    return NULL;
}

struct Shader* fetch_shader(const char* name) {
    load_shader(name);
    return get_shader(name);
}

void destroy_shader(struct Shader* shader) {
    if (shaders == shader)
        shaders = shader->previous;
    if (shader->previous != NULL)
        shader->previous->next = shader->next;
    if (shader->next != NULL)
        shader->next->previous = shader->previous;

    glDeleteProgram(shader->program);
    SDL_DestroyProperties(shader->uniforms);

    INFO("Freed shader \"%s\" (%u)", shader->name, shader->hid);
    destroy_handle(shader_handles, shader->hid);
    lame_free(&shader);
}

void clear_shaders(int teardown) {
    struct Shader* shader = shaders;

    while (shader != NULL) {
        struct Shader* it = shader->previous;
        if (!shader->transient || teardown)
            destroy_shader(shader);
        shader = it;
    }
}

// Textures
void load_texture(const char* name) {
    if (get_texture(name) != NULL)
        return;

    SDL_snprintf(path_helper, sizeof(path_helper), "textures/%s.*", name);
    const char* file = get_file(path_helper, ".json");
    if (file == NULL) {
        WARN("Texture \"%s\" not found", name);
        return;
    }

    SDL_Surface* surface = IMG_Load(file);
    if (surface == NULL)
        FATAL("Failed to load image for texture \"s\": %s", SDL_GetError());

    // Texture struct
    struct Texture* texture = lame_alloc(sizeof(struct Texture));

    // General
    texture->hid = (TextureID)create_handle(texture_handles, texture);
    SDL_strlcpy(texture->name, name, sizeof(texture->name));
    if (textures != NULL)
        textures->next = texture;
    texture->previous = textures;
    texture->next = NULL;
    textures = texture;

    // Data
    texture->size[0] = surface->w;
    texture->size[1] = surface->h;
    texture->offset[0] = texture->offset[1] = 0;
    texture->uvs[0] = texture->uvs[1] = 0;
    texture->uvs[2] = texture->uvs[3] = 1;

    glGenTextures(1, &texture->texture);
    glBindTexture(GL_TEXTURE_2D, texture->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_Surface* surface_rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (surface_rgba == NULL)
        FATAL("Failed to load image for texture \"s\": %s", SDL_GetError());
    SDL_DestroySurface(surface);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, surface_rgba->w, surface_rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface_rgba->pixels
    );
    SDL_DestroySurface(surface_rgba);

    INFO("Loaded texture \"%s\" (%u)", name, texture->hid);
}

struct Texture* fetch_texture(const char* name) {
    load_texture(name);
    return get_texture(name);
}

TextureID fetch_texture_hid(const char* name) {
    load_texture(name);
    return get_texture_hid(name);
}

struct Texture* get_texture(const char* name) {
    for (struct Texture* texture = textures; texture != NULL; texture = texture->previous)
        if (SDL_strcmp(texture->name, name) == 0)
            return texture;
    return NULL;
}

TextureID get_texture_hid(const char* name) {
    for (struct Texture* texture = textures; texture != NULL; texture = texture->previous)
        if (SDL_strcmp(texture->name, name) == 0)
            return texture->hid;
    return 0;
}

extern struct Texture* hid_to_texture(TextureID hid) {
    return (struct Texture*)hid_to_pointer(texture_handles, (TextureID)hid);
}

void destroy_texture(struct Texture* texture) {
    if (textures == texture)
        textures = texture->previous;
    if (texture->previous != NULL)
        texture->previous->next = texture->next;
    if (texture->next != NULL)
        texture->next->previous = texture->previous;

    glDeleteTextures(1, &texture->texture);

    INFO("Freed texture \"%s\" (%u)", texture->name, texture->hid);
    destroy_handle(texture_handles, texture->hid);
    lame_free(&texture);
}

void destroy_texture_hid(TextureID hid) {
    struct Texture* texture = (struct Texture*)hid_to_pointer(texture_handles, (HandleID)hid);
    if (texture != NULL)
        destroy_texture(texture);
}

void clear_textures(int teardown) {
    struct Texture* texture = textures;

    while (texture != NULL) {
        struct Texture* it = texture->previous;
        if (!texture->transient || teardown)
            destroy_texture(texture);
        texture = it;
    }
}

// Sounds
struct Sound* create_sound(const char* name) {
    struct Sound* sound = lame_alloc(sizeof(struct Sound));

    // General
    sound->hid = (SoundID)create_handle(sound_handles, sound);
    SDL_strlcpy(sound->name, name, sizeof(sound->name));
    sound->transient = 0;
    if (sounds != NULL)
        sounds->next = sound;
    sound->previous = sounds;
    sound->next = NULL;
    sounds = sound;

    // Data
    sound->samples = NULL;
    sound->num_samples = 0;
    sound->gain = 1;
    sound->pitch[0] = sound->pitch[1] = 1;

    return sound;
}

void load_sound(const char* name) {
    if (get_sound(name) != NULL)
        return;

    // Find a JSON for definitions
    SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.json", name);
    const char* file = get_file(path_helper, NULL);
    if (file == NULL) {
        // Fall back to just using a sample
        SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", name);
        if ((file = get_file(path_helper, ".json")) == NULL) {
            WARN("Sound \"%s\" not found", name);
            return;
        }

        struct Sound* sound = create_sound(name);

        // Data
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;
        load_sample(file, &sound->samples[0]);

        INFO("Loaded sound \"%s\" (%u)", name, sound->hid);
        return;
    }

    yyjson_doc* json = yyjson_read_file(file, JSON_FLAGS, NULL, NULL);
    if (json == NULL) {
        WTF("Failed to load Sound \"%s.json\"", name);
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root)) {
        WTF("Expected root object in \"%s.json\", got %s", name, yyjson_get_type_desc(root));
        yyjson_doc_free(json);
        return;
    }

    // Allocate empty sound at this point
    struct Sound* sound = create_sound(name);

    // Samples
    yyjson_val* value = yyjson_obj_get(root, "sample");
    if (yyjson_is_str(value)) {
        sound->samples = lame_alloc(sizeof(struct Sample*));
        sound->num_samples = 1;

        SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", yyjson_get_str(value));
        if ((file = get_file(path_helper, ".json")) == NULL) {
            sound->samples[0] = NULL;
            WARN("Sample \"%s\" not found", path_helper);
        } else {
            load_sample(file, &sound->samples[0]);
        }
    } else if (yyjson_is_arr(value)) {
        sound->num_samples = yyjson_arr_size(value);
        sound->samples = lame_alloc(sound->num_samples * sizeof(struct Sample*));

        size_t i, n;
        yyjson_val* entry;
        yyjson_arr_foreach(value, i, n, entry) {
            if (yyjson_is_null(entry)) {
                sound->samples[i] = NULL;
                continue;
            } else if (!yyjson_is_str(entry)) {
                sound->samples[i] = NULL;
                WTF("Expected \"sample\" index %u as string or null, got %s", i, yyjson_get_type_desc(entry));
                continue;
            }

            SDL_snprintf(path_helper, sizeof(path_helper), "sounds/%s.*", yyjson_get_str(entry));
            if ((file = get_file(path_helper, ".json")) == NULL) {
                sound->samples[i] = NULL;
                WARN("Sample \"%s\" not found", path_helper);
                continue;
            }
            load_sample(file, &sound->samples[i]);
        }
    } else if (!yyjson_is_null(value)) {
        WTF("Expected \"sample\" as string, array or null in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Gain
    if (yyjson_is_num(value = yyjson_obj_get(root, "gain")))
        sound->gain = (float)yyjson_get_num(value);

    // Pitch
    value = yyjson_obj_get(root, "pitch");
    if (yyjson_is_arr(value)) {
        if (yyjson_arr_size(value) >= 2) {
            sound->pitch[0] = yyjson_get_num(yyjson_arr_get(value, 0));
            sound->pitch[1] = yyjson_get_num(yyjson_arr_get(value, 1));
        }
    } else if (yyjson_is_num(value)) {
        sound->pitch[0] = sound->pitch[1] = yyjson_get_num(value);
    }

    yyjson_doc_free(json);

    INFO("Loaded sound \"%s\" (%u)", name, sound->hid);
}

struct Sound* fetch_sound(const char* name) {
    load_sound(name);
    return get_sound(name);
}

SoundID fetch_sound_hid(const char* name) {
    load_sound(name);
    return get_sound_hid(name);
}

struct Sound* get_sound(const char* name) {
    for (struct Sound* sound = sounds; sound != NULL; sound = sound->previous)
        if (SDL_strcmp(sound->name, name) == 0)
            return sound;
    return NULL;
}

SoundID get_sound_hid(const char* name) {
    for (struct Sound* sound = sounds; sound != NULL; sound = sound->previous)
        if (SDL_strcmp(sound->name, name) == 0)
            return sound->hid;
    return 0;
}

extern struct Sound* hid_to_sound(SoundID hid) {
    return (struct Sound*)hid_to_pointer(sound_handles, (HandleID)hid);
}

void destroy_sound(struct Sound* sound) {
    if (sounds == sound)
        sounds = sound->previous;
    if (sound->previous != NULL)
        sound->previous->next = sound->next;
    if (sound->next != NULL)
        sound->next->previous = sound->previous;

    if (sound->samples != NULL) {
        for (size_t i = 0; i < sound->num_samples; i++)
            if (sound->samples[i] != NULL)
                destroy_sample(sound->samples[i]);
        lame_free(&sound->samples);
    }

    INFO("Freed sound \"%s\" (%u)", sound->name, sound->hid);
    destroy_handle(sound_handles, sound->hid);
    lame_free(&sound);
}

void destroy_sound_hid(SoundID hid) {
    struct Sound* sound = (struct Sound*)hid_to_pointer(sound_handles, (HandleID)hid);
    if (sound != NULL)
        destroy_sound(sound);
}

void clear_sounds(int teardown) {
    struct Sound* sound = sounds;

    while (sound != NULL) {
        struct Sound* it = sound->previous;
        if (!sound->transient || teardown)
            destroy_sound(sound);
        sound = it;
    }
}

// Music
void load_track(const char* name) {
    if (get_track(name) != NULL)
        return;

    SDL_snprintf(path_helper, sizeof(path_helper), "music/%s.*", name);
    const char* track_file = get_file(path_helper, ".json");
    if (track_file == NULL) {
        WARN("Track \"%s\" not found", name);
        return;
    }

    struct Track* track = lame_alloc(sizeof(struct Track));

    // General
    track->hid = (TrackID)create_handle(track_handles, track);
    SDL_strlcpy(track->name, name, sizeof(track->name));
    track->transient = 0;
    if (music != NULL)
        music->next = track;
    track->previous = music;
    track->next = NULL;
    music = track;

    // Data
    load_stream(track_file, &track->stream);

    INFO("Loaded track \"%s\" (%u)", name, track->hid);
}

struct Track* fetch_track(const char* name) {
    load_track(name);
    return get_track(name);
}

TrackID fetch_track_hid(const char* name) {
    load_track(name);
    return get_track_hid(name);
}

struct Track* get_track(const char* name) {
    for (struct Track* track = music; track != NULL; track = track->previous)
        if (SDL_strcmp(track->name, name) == 0)
            return track;
    return NULL;
}

TrackID get_track_hid(const char* name) {
    for (struct Track* track = music; track != NULL; track = track->previous)
        if (SDL_strcmp(track->name, name) == 0)
            return track->hid;
    return 0;
}

extern struct Track* hid_to_track(TrackID hid) {
    return (struct Track*)hid_to_pointer(track_handles, (HandleID)hid);
}

void destroy_track(struct Track* track) {
    if (music == track)
        music = track->previous;
    if (track->previous != NULL)
        track->previous->next = track->next;
    if (track->next != NULL)
        track->next->previous = track->previous;

    destroy_stream(track->stream);
    INFO("Freed track \"%s\" (%u)", track->name, track->hid);
    destroy_handle(track_handles, track->hid);
    lame_free(&track);
}

void destroy_track_hid(TrackID hid) {
    struct Track* track = (struct Track*)hid_to_pointer(track_handles, (HandleID)hid);
    if (track != NULL)
        destroy_track(track);
}

void clear_music(int teardown) {
    struct Track* track = music;

    while (track != NULL) {
        struct Track* it = track->previous;
        if (!track->transient || teardown)
            destroy_track(track);
        track = it;
    }
}
