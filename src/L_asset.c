#include <SDL3_image/SDL_image.h>

#include "L_asset.h"
#include "L_file.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_mod.h"

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
    clear_assets(1);

    CLOSE_POINTER(shader_handles, destroy_fixture);
    CLOSE_POINTER(texture_handles, destroy_fixture);
    CLOSE_POINTER(material_handles, destroy_fixture);
    CLOSE_POINTER(model_handles, destroy_fixture);
    CLOSE_POINTER(font_handles, destroy_fixture);
    CLOSE_POINTER(sound_handles, destroy_fixture);
    CLOSE_POINTER(track_handles, destroy_fixture);

    INFO("Closed");
}

void clear_assets(int teardown) {
    clear_shaders(teardown);
    clear_textures(teardown);
    clear_fonts(teardown);
    clear_sounds(teardown);
    clear_music(teardown);
}

static char asset_file_helper[FILE_PATH_MAX];

// Shaders
void load_shader(const char* name) {
    if (get_shader(name) != NULL)
        return;

    // Vertex shader
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "shaders/%s.vsh", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Vertex shader for \"%s\" not found", name);
        return;
    }

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);

    GLchar* code = SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Shader \"%s\" vertex load fail: %s", name, SDL_GetError());
    glShaderSource(vertex, 1, (const GLchar* const*)&code, NULL);
    glCompileShader(vertex);

    GLint success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024); // Why not reuse the code string?
        glGetShaderInfoLog(vertex, 1024, NULL, code);
        FATAL("Shader \"%s\" vertex fail:\n%s", name, code);
    }
    lame_free(&code);

    // Fragment shader
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "shaders/%s.fsh", name);
    file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Fragment shader for \"%s\" not found", name);
        glDeleteShader(vertex);
        return;
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

    code = (GLchar*)SDL_LoadFile(file, NULL);
    if (code == NULL)
        FATAL("Shader \"%s\" fragment load fail: %s", name, SDL_GetError());
    glShaderSource(fragment, 1, (const GLchar* const*)&code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        lame_realloc(&code, 1024);
        glGetShaderInfoLog(fragment, 1024, NULL, code);
        FATAL("Shader \"%s\" fragment fail:\n%s", name, code);
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
        FATAL("Shader \"%s\" program fail:\n%s", name, code);
    }
    lame_free(&code); // Now we're good

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Uniforms
    if ((shader->uniforms = SDL_CreateProperties()) == 0)
        FATAL("Shader \"%s\" uniforms fail: %s", name, SDL_GetError());
    GLsizei unum, ulen;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &unum);
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &ulen);
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

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "textures/%s.*", name);
    const char* file = get_mod_file(asset_file_helper, ".json");
    if (file == NULL) {
        WARN("Texture \"%s\" not found", name);
        return;
    }

    SDL_Surface* surface = IMG_Load(file);
    if (surface == NULL) {
        WTF("Texture \"%s\" image fail: %s", SDL_GetError());
        return;
    }

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

    // Inheritance
    texture->parent = NULL;
    texture->children = NULL;
    texture->num_children = 0;

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

    GLint format;
    switch (surface->format) {
        default: {
            SDL_Surface* temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (temp == NULL)
                FATAL("Texture \"%s\" image conversion fail: %s", SDL_GetError());
            SDL_DestroySurface(surface);
            surface = temp;

            format = GL_RGBA8;
            break;
        }

        case SDL_PIXELFORMAT_RGB24:
            format = GL_RGB8;
            break;
        case SDL_PIXELFORMAT_RGB48:
            format = GL_RGB16;
            break;
        case SDL_PIXELFORMAT_RGB48_FLOAT:
            format = GL_RGB16F;
            break;
        case SDL_PIXELFORMAT_RGB96_FLOAT:
            format = GL_RGB32F;
            break;

        case SDL_PIXELFORMAT_RGBA32:
            format = GL_RGBA8;
            break;
        case SDL_PIXELFORMAT_RGBA64:
            format = GL_RGBA16;
            break;
        case SDL_PIXELFORMAT_RGBA64_FLOAT:
            format = GL_RGBA16F;
            break;
        case SDL_PIXELFORMAT_RGBA128_FLOAT:
            format = GL_RGBA32F;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_DestroySurface(surface);

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

struct Texture* hid_to_texture(TextureID hid) {
    return (struct Texture*)hid_to_pointer(texture_handles, (TextureID)hid);
}

void destroy_texture(struct Texture* texture) {
    if (textures == texture)
        textures = texture->previous;
    if (texture->previous != NULL)
        texture->previous->next = texture->next;
    if (texture->next != NULL)
        texture->next->previous = texture->previous;

    if (texture->children != NULL) {
        for (int i = 0; i < texture->num_children; i++)
            destroy_texture_hid(texture->children[i]);
        lame_free(&texture->children);
    }
    if (texture->parent == NULL)
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

// Fonts
void load_font(const char* name) {
    if (get_font(name) != NULL)
        return;

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "fonts/%s.json", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        WARN("Font \"%s\" not found", name);
        return;
    }

    yyjson_doc* json = load_json(file);
    if (json == NULL) {
        WTF("Font \"%s\" load fail", name);
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(json);
    if (!yyjson_is_obj(root)) {
        WTF("Expected root object in \"%s.json\", got %s", name, yyjson_get_type_desc(root));
        yyjson_doc_free(json);
        return;
    }

    struct Font* font = lame_alloc(sizeof(struct Font));

    // General
    font->hid = (FontID)create_handle(font_handles, font);
    SDL_strlcpy(font->name, name, sizeof(font->name));
    font->transient = 0;
    if (fonts != NULL)
        fonts->next = font;
    font->previous = fonts;
    font->next = NULL;
    fonts = font;

    // Data
    font->texture = 0;
    font->size = 0;
    font->glyphs = NULL;
    font->num_glyphs = 0;

    // Texture
    struct Texture* texture;
    yyjson_val* value = yyjson_obj_get(root, "texture");
    if (yyjson_is_str(value)) {
        if ((texture = fetch_texture(yyjson_get_str(value))) == NULL)
            FATAL("Font texture \"%s\" not found", name);
        font->texture = texture->hid;
    } else {
        FATAL("Expected \"texture\" as string in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    }

    // Glyphs
    value = yyjson_obj_get(root, "size");
    if (!yyjson_is_uint(value))
        FATAL("Expected \"size\" as uint in \"%s.json\", got %s", name, yyjson_get_type_desc(value));
    if ((font->size = yyjson_get_uint(value)) <= 0)
        FATAL("Expected non-zero size for font \"%s\"", name);

    value = yyjson_obj_get(root, "glyphs");
    if (!yyjson_is_obj(value))
        FATAL("Expected \"glyphs\" as object in \"%s.json\", got %s", name, yyjson_get_type_desc(value));

    size_t gldef = 0;
    size_t i, n;
    yyjson_val *key, *val;
    yyjson_obj_foreach(value, i, n, key, val) {
        if (!yyjson_is_obj(val))
            FATAL(
                "Expected glyph \"%c\" as object in \"%s.json\", got %s", yyjson_get_str(key)[0], name,
                yyjson_get_type_desc(value)
            );

        const char* character = yyjson_get_str(key);
        size_t gid = SDL_StepUTF8(&character, NULL);
        if (font->num_glyphs <= gid) {
            if (font->glyphs == NULL) {
                font->num_glyphs = gid + 1;
                font->glyphs = lame_alloc(font->num_glyphs * sizeof(struct Glyph*));
                lame_set(font->glyphs, 0, font->num_glyphs * sizeof(struct Glyph*));
            } else {
                size_t old_num = font->num_glyphs;
                font->num_glyphs = gid + 1;
                lame_realloc(&font->glyphs, font->num_glyphs * sizeof(struct Glyph*));
                lame_set(font->glyphs + old_num, 0, (font->num_glyphs - old_num) * sizeof(struct Glyph*));
            }
        }

        struct Glyph* glyph = font->glyphs[gid];
        if (glyph == NULL) {
            glyph = font->glyphs[gid] = lame_alloc(sizeof(struct Glyph));
            ++gldef;
        } else
            WARN("Font \"%s\" overwriting glyph \"%c\"", name, gid);
        glyph->size[0] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "width"));
        glyph->size[1] = (GLfloat)yyjson_get_uint(yyjson_obj_get(val, "height"));
        glyph->offset[0] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "x_offset"));
        glyph->offset[1] = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "y_offset"));
        glyph->uvs[0] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "x"))) / texture->size[0];
        glyph->uvs[1] = ((GLfloat)yyjson_get_uint(yyjson_obj_get(val, "y"))) / texture->size[1];
        glyph->uvs[2] = glyph->uvs[0] + (glyph->size[0] / texture->size[0]);
        glyph->uvs[3] = glyph->uvs[1] + (glyph->size[1] / texture->size[1]);
        glyph->advance = (GLfloat)yyjson_get_num(yyjson_obj_get(val, "advance"));
    }

    yyjson_doc_free(json);

    INFO("Loaded font \"%s\" (%u, %u/%u glyphs)", name, font->hid, gldef, font->num_glyphs);
}

struct Font* fetch_font(const char* name) {
    load_font(name);
    return get_font(name);
}

FontID fetch_font_hid(const char* name) {
    load_font(name);
    return get_font_hid(name);
}

struct Font* get_font(const char* name) {
    for (struct Font* font = fonts; font != NULL; font = font->previous)
        if (SDL_strcmp(font->name, name) == 0)
            return font;
    return NULL;
}

FontID get_font_hid(const char* name) {
    for (struct Font* font = fonts; font != NULL; font = font->previous)
        if (SDL_strcmp(font->name, name) == 0)
            return font->hid;
    return 0;
}

struct Font* hid_to_font(FontID hid) {
    return (struct Font*)hid_to_pointer(font_handles, (FontID)hid);
}

void destroy_font(struct Font* font) {
    if (fonts == font)
        fonts = font->previous;
    if (font->previous != NULL)
        font->previous->next = font->next;
    if (font->next != NULL)
        font->next->previous = font->previous;

    if (font->glyphs != NULL) {
        for (size_t i = 0; i < font->num_glyphs; i++)
            FREE_POINTER(font->glyphs[i]);
        lame_free(&font->glyphs);
    }

    INFO("Freed font \"%s\" (%u)", font->name, font->hid);
    destroy_handle(font_handles, font->hid);
    lame_free(&font);
}

void destroy_font_hid(FontID hid) {
    struct Font* font = (struct Font*)hid_to_pointer(font_handles, (HandleID)hid);
    if (font != NULL)
        destroy_font(font);
}

void clear_fonts(int teardown) {
    struct Font* font = fonts;
    while (font != NULL) {
        struct Font* it = font->previous;
        if (!font->transient || teardown)
            destroy_font(font);
        font = it;
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
    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.json", name);
    const char* file = get_mod_file(asset_file_helper, NULL);
    if (file == NULL) {
        // Fall back to just using a sample
        SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", name);
        if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
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

    yyjson_doc* json = load_json(file);
    if (json == NULL) {
        WTF("Sound \"%s\" load fail", name);
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

        SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(value));
        if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
            sound->samples[0] = NULL;
            WARN("Sample \"%s\" not found", asset_file_helper);
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

            SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "sounds/%s.*", yyjson_get_str(entry));
            if ((file = get_mod_file(asset_file_helper, ".json")) == NULL) {
                sound->samples[i] = NULL;
                WARN("Sample \"%s\" not found", asset_file_helper);
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

struct Sound* hid_to_sound(SoundID hid) {
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

    SDL_snprintf(asset_file_helper, sizeof(asset_file_helper), "music/%s.*", name);
    const char* track_file = get_mod_file(asset_file_helper, ".json");
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

struct Track* hid_to_track(TrackID hid) {
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
