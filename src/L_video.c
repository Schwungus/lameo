#include <SDL3/SDL_timer.h>

#include "L_actor.h"
#include "L_internal.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_player.h"
#include "L_ui.h"
#include "L_video.h"

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

static struct Display display = {-1, -1, 0};
static uint16_t framerate = 0;
static uint64_t last_cap_time = 0;
static float cap_wait = 0;
static uint64_t draw_time = 0;

static GLuint blank_texture = 0;

static enum RenderTypes render_stage = RT_MAIN;
static struct MainBatch main_batch = {0};
static struct WorldBatch world_batch = {0};
static struct ActorCamera* active_camera = NULL;

static struct Shader* default_shaders[RT_SIZE] = {NULL};
static struct Shader* current_shader = NULL;
static struct Font* default_font = NULL;
static struct Surface* current_surface = NULL;

static mat4 model_matrix, view_matrix, projection_matrix, mvp_matrix;

void video_init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Window
    window = SDL_CreateWindow("lameo", DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, SDL_WINDOW_OPENGL);
    if (window == NULL)
        FATAL("Window fail: %s", SDL_GetError());

    // OpenGL
    gpu = SDL_GL_CreateContext(window);
    if (gpu == NULL)
        FATAL("GPU fail: %s", SDL_GetError());
    SDL_GL_SetSwapInterval(0);

    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0)
        FATAL("Failed to load OpenGL functions");
    INFO("GLAD version: %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    INFO("OpenGL vendor: %s", glGetString(GL_VENDOR));
    INFO("OpenGL version: %s", glGetString(GL_VERSION));
    INFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
    INFO("OpenGL shading language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Display info
    display.width = DEFAULT_DISPLAY_WIDTH;
    display.height = DEFAULT_DISPLAY_HEIGHT;
    display.fullscreen = FSM_WINDOWED;
    display.vsync = false;

    // Rendering
    glm_mat4_identity(model_matrix);
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(projection_matrix);
    glm_mat4_identity(mvp_matrix);

    // Blank texture
    glGenTextures(1, &blank_texture);
    glBindTexture(GL_TEXTURE_2D, blank_texture);
    const uint8_t pixel[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // Main batch
    glGenVertexArrays(1, &main_batch.vao);
    glBindVertexArray(main_batch.vao);
    glEnableVertexArrayAttrib(main_batch.vao, VATT_POSITION);
    glVertexArrayAttribFormat(main_batch.vao, VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(main_batch.vao, VATT_COLOR);
    glVertexArrayAttribFormat(main_batch.vao, VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(main_batch.vao, VATT_UV);
    glVertexArrayAttribFormat(main_batch.vao, VATT_UV, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2);

    main_batch.vertex_count = 0;
    main_batch.vertex_capacity = 3;
    main_batch.vertices = lame_alloc(main_batch.vertex_capacity * sizeof(struct MainVertex));

    glGenBuffers(1, &main_batch.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct MainVertex) * main_batch.vertex_capacity, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(VATT_POSITION);
    glVertexAttribPointer(
        VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct MainVertex), (void*)offsetof(struct MainVertex, position)
    );

    glEnableVertexAttribArray(VATT_COLOR);
    glVertexAttribPointer(
        VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct MainVertex), (void*)offsetof(struct MainVertex, color)
    );

    glEnableVertexAttribArray(VATT_UV);
    glVertexAttribPointer(
        VATT_UV, 2, GL_FLOAT, GL_FALSE, sizeof(struct MainVertex), (void*)offsetof(struct MainVertex, uv)
    );

    main_batch.color[0] = main_batch.color[1] = main_batch.color[2] = main_batch.color[3] = 1;

    main_batch.stencil[0] = main_batch.stencil[1] = main_batch.stencil[2] = 1;
    main_batch.stencil[3] = 0;

    main_batch.texture = blank_texture;
    main_batch.alpha_test = 0;

    main_batch.blend_src[0] = main_batch.blend_src[1] = GL_SRC_ALPHA;
    main_batch.blend_dest[0] = GL_ONE_MINUS_SRC_ALPHA;
    main_batch.blend_dest[1] = GL_ONE;

    main_batch.filter = false;

    // World batch
    glGenVertexArrays(1, &world_batch.vao);
    glBindVertexArray(world_batch.vao);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_POSITION);
    glVertexArrayAttribFormat(world_batch.vao, VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_NORMAL);
    glVertexArrayAttribFormat(world_batch.vao, VATT_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_COLOR);
    glVertexArrayAttribFormat(world_batch.vao, VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_UV);
    glVertexArrayAttribFormat(world_batch.vao, VATT_UV, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_BONE_INDEX);
    glVertexArrayAttribFormat(world_batch.vao, VATT_BONE_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_BONE_WEIGHT);
    glVertexArrayAttribFormat(world_batch.vao, VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);

    world_batch.vertex_count = 0;
    world_batch.vertex_capacity = 3;
    world_batch.vertices = lame_alloc(world_batch.vertex_capacity * sizeof(struct WorldVertex));

    glGenBuffers(1, &world_batch.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, world_batch.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct WorldVertex) * world_batch.vertex_capacity, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(VATT_POSITION);
    glVertexAttribPointer(
        VATT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex), (void*)offsetof(struct WorldVertex, position)
    );

    glEnableVertexAttribArray(VATT_NORMAL);
    glVertexAttribPointer(
        VATT_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex), (void*)offsetof(struct WorldVertex, normal)
    );

    glEnableVertexAttribArray(VATT_COLOR);
    glVertexAttribPointer(
        VATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct WorldVertex), (void*)offsetof(struct WorldVertex, color)
    );

    glEnableVertexAttribArray(VATT_UV);
    glVertexAttribPointer(
        VATT_UV, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex), (void*)offsetof(struct WorldVertex, uv)
    );

    glEnableVertexAttribArray(VATT_BONE_INDEX);
    glVertexAttribPointer(
        VATT_BONE_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct WorldVertex),
        (void*)offsetof(struct WorldVertex, bone_index)
    );

    glEnableVertexAttribArray(VATT_BONE_WEIGHT);
    glVertexAttribPointer(
        VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
        (void*)offsetof(struct WorldVertex, bone_weight)
    );

    world_batch.color[0] = world_batch.color[1] = world_batch.color[2] = world_batch.color[3] = 1;

    world_batch.stencil[0] = world_batch.stencil[1] = world_batch.stencil[2] = 1;
    world_batch.stencil[3] = 0;

    world_batch.texture = blank_texture;
    world_batch.alpha_test = 0;

    world_batch.blend_src[0] = world_batch.blend_src[1] = GL_SRC_ALPHA;
    world_batch.blend_dest[0] = GL_ONE_MINUS_SRC_ALPHA;
    world_batch.blend_dest[1] = GL_ONE;

    world_batch.filter = true;

    glEnable(GL_BLEND);

    // Shaders
    if (default_shaders[RT_MAIN] == NULL && (default_shaders[RT_MAIN] = fetch_shader("main")) == NULL)
        FATAL("Main shader \"main\" not found");
    if (default_shaders[RT_WORLD] == NULL && (default_shaders[RT_WORLD] = fetch_shader("world")) == NULL)
        FATAL("Main shader \"world\" not found");

    // Fonts
    if ((default_font = fetch_font("main")) == NULL)
        FATAL("Main font \"main\" not found");
    // GROSS HACK: Font texture has to be manually made transient
    default_font->transient = true;
    struct Texture* fontex = hid_to_texture(default_font->texture);
    if (fontex != NULL)
        fontex->transient = true;

    INFO("Opened");
}

void video_update() {
    draw_time = SDL_GetTicks();
    if (framerate > 0) {
        cap_wait += (float)(draw_time - last_cap_time) * ((float)framerate / 1000.);
        last_cap_time = draw_time;
        if (cap_wait < 1)
            return;
        cap_wait -= SDL_floorf(cap_wait);
    } else {
        last_cap_time = draw_time;
    }

    set_surface(NULL);
    clear_color(0, 0, 0, 1);
    set_render_stage(RT_MAIN);
    set_shader(NULL);

    /*const float scalew = (float)display.width / (float)DEFAULT_DISPLAY_WIDTH;
    const float scaleh = (float)display.height / (float)DEFAULT_DISPLAY_HEIGHT;
    const float scale = SDL_min(scalew, scaleh);

    const float width = (float)DEFAULT_DISPLAY_WIDTH * scale;
    const float height = (float)DEFAULT_DISPLAY_HEIGHT * scale;
    glViewport(((float)display.width - width) / 2, ((float)display.height - height) / 2, width, height);*/

    if (get_load_state() != LOAD_NONE) {
        const char* loading = localized("loading");
        main_string(
            loading, NULL, 16, ((GLfloat)DEFAULT_DISPLAY_WIDTH - string_width(loading, NULL, 16)) / 2,
            ((GLfloat)DEFAULT_DISPLAY_HEIGHT - string_height(loading, 16)) / 2, 0
        );
    } else {
        struct ActorCamera* camera = active_camera;
        if (camera == NULL) {
            // TODO: Splitscreen
            struct Player* player = get_active_players();
            while (player != NULL) {
                if (player->actor != NULL && (camera = player->actor->camera) != NULL)
                    break;
                player = player->previous_active;
            }
        }
        if (camera != NULL)
            main_surface(render_camera(camera, display.width, display.height), 0, 0, 0);

        struct Player* player = get_active_players();
        while (player != NULL) {
            if (player->room != NULL && player->room->master == player) {
                struct Actor* actor = player->room->actors;
                while (actor != NULL) {
                    if ((actor->flags & AF_VISIBLE) && actor->type->draw_ui != LUA_NOREF)
                        execute_ref_in(actor->type->draw_ui, actor->userdata, actor->type->name);
                    actor = actor->previous_neighbor;
                }
            }
            player = player->previous_active;
        }

        const struct UI* ui_top = get_ui_top();
        if (ui_top != NULL && ui_top->type->draw != LUA_NOREF)
            execute_ref_in(ui_top->type->draw, ui_top->userdata, ui_top->type->name);
    }

    submit_batch();

    // If Steam Overlay hooks on to the application, MSVC debugger may cause a
    // breakpoint here. Otherwise the program itself runs without a problem.
    SDL_GL_SwapWindow(window);
}

void video_teardown() {
    glDeleteTextures(1, &blank_texture);

    glDeleteVertexArrays(1, &main_batch.vao);
    glDeleteBuffers(1, &main_batch.vbo);
    lame_free(&main_batch.vertices);

    glDeleteVertexArrays(1, &world_batch.vao);
    glDeleteBuffers(1, &world_batch.vbo);
    lame_free(&world_batch.vertices);

    CLOSE_POINTER(gpu, SDL_GL_DestroyContext);
    CLOSE_POINTER(window, SDL_DestroyWindow);

    INFO("Closed");
}

// Display
const struct Display* get_display() {
    return &display;
}

void set_display(int width, int height, enum FullscreenModes fullscreen, bool vsync) {
    if (window == NULL || (display.width == width && display.height == height && display.fullscreen == fullscreen &&
                           display.vsync == vsync))
        return;

    // Size
    if (display.width <= 0 || display.height <= 0) {
        display.width = DEFAULT_DISPLAY_WIDTH;
        display.height = DEFAULT_DISPLAY_HEIGHT;
    } else {
        SDL_DisplayID did = SDL_GetDisplayForWindow(window);
        const SDL_DisplayMode* monitor = (display.fullscreen != FSM_WINDOWED && fullscreen == FSM_WINDOWED)
                                             ? SDL_GetDesktopDisplayMode(did)
                                             : SDL_GetCurrentDisplayMode(did);
        if (monitor != NULL) {
            display.width = SDL_min(width, monitor->w);
            display.height = SDL_min(height, monitor->h);
        } else {
            display.width = width;
            display.height = height;
        }
    }
    SDL_SetWindowSize(window, display.width, display.height);

    // Fullscreen
    display.fullscreen = fullscreen;
    if (display.fullscreen > FSM_EXCLUSIVE_FULLSCREEN)
        display.fullscreen = FSM_EXCLUSIVE_FULLSCREEN;

    // MEMORY LEAK: In exclusive fullscreen, Windows will leak ~220 bytes every
    //              time you tab out and back in.
    //              https://github.com/libsdl-org/SDL/issues/13233
    // Pattern: <C D D           > 43 00 44 00 44 00 00 00 00 00 00 00 00 00 00 00
    /*if (display.fullscreen == FSM_EXCLUSIVE_FULLSCREEN) {
        SDL_DisplayMode dm;
        SDL_SetWindowFullscreenMode(
            window, SDL_GetClosestFullscreenDisplayMode(
                        SDL_GetDisplayForWindow(window), display.width, display.height, 0, true, &dm
                    )
                        ? &dm
                        : NULL
        );
    } else {
        SDL_SetWindowFullscreenMode(window, NULL);
    }*/
    SDL_SetWindowFullscreen(window, display.fullscreen != FSM_WINDOWED);

    // Vsync
    SDL_GL_SetSwapInterval(display.vsync = vsync);

    // Center and wait
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RestoreWindow(window);
    SDL_SyncWindow(window);

    SDL_GetWindowSizeInPixels(window, &display.width, &display.height);
    INFO("Display set to %dx%d, mode %d, vsync %d", display.width, display.height, display.fullscreen, display.vsync);
}

void set_framerate(uint16_t fps) {
    if (framerate == fps)
        return;
    if ((framerate = fps) > 0)
        INFO("Capped framerate to %d FPS", fps);
    else
        INFO("Uncapped framerate");
}

uint64_t get_draw_time() {
    return draw_time;
}

// Shaders 'n' Uniforms
void set_shader(struct Shader* shader) {
    struct Shader* target = (shader == NULL) ? default_shaders[render_stage] : shader;
    if (current_shader != target) {
        submit_batch();
        current_shader = target;
        glUseProgram(target->program);
    }
}

void set_uint_uniform(const char* name, GLuint value) {
    glUniform1ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_uvec2_uniform(const char* name, GLuint x, GLuint y) {
    glUniform2ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

void set_uvec3_uniform(const char* name, GLuint x, GLuint y, GLuint z) {
    glUniform3ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

void set_uvec4_uniform(const char* name, GLuint x, GLuint y, GLuint z, GLuint w) {
    glUniform4ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

void set_int_uniform(const char* name, GLint value) {
    glUniform1i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_ivec2_uniform(const char* name, GLint x, GLint y) {
    glUniform2i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

void set_ivec3_uniform(const char* name, GLint x, GLint y, GLint z) {
    glUniform3i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

void set_ivec4_uniform(const char* name, GLint x, GLint y, GLint z, GLint w) {
    glUniform4i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

void set_float_uniform(const char* name, GLfloat value) {
    glUniform1f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_vec2_uniform(const char* name, GLfloat x, GLfloat y) {
    glUniform2f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

void set_vec3_uniform(const char* name, GLfloat x, GLfloat y, GLfloat z) {
    glUniform3f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

void set_vec4_uniform(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    glUniform4f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

void set_mat2_uniform(const char* name, mat2* matrix) {
    glUniformMatrix2fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

void set_mat3_uniform(const char* name, mat3* matrix) {
    glUniformMatrix3fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

void set_mat4_uniform(const char* name, mat4* matrix) {
    glUniformMatrix4fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

// Render stages
void set_render_stage(enum RenderTypes type) {
    if (render_stage != type) {
        submit_batch();
        render_stage = type;
    }
}

void submit_batch() {
    switch (render_stage) {
        default:
            break;
        case RT_MAIN:
            submit_main_batch();
            break;
        case RT_WORLD:
            submit_world_batch();
            break;
    }
}

// Main
void submit_main_batch() {
    if (main_batch.vertex_count <= 0)
        return;

    // Apply matrices
    set_mat4_uniform("u_model_matrix", &model_matrix);
    set_mat4_uniform("u_view_matrix", &view_matrix);
    set_mat4_uniform("u_projection_matrix", &projection_matrix);
    set_mat4_uniform("u_mvp_matrix", &mvp_matrix);

    glBindVertexArray(main_batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct MainVertex) * main_batch.vertex_count, main_batch.vertices);

    // Apply stencil
    set_vec4_uniform(
        "u_stencil", main_batch.stencil[0], main_batch.stencil[1], main_batch.stencil[2], main_batch.stencil[3]
    );

    // Apply texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, main_batch.texture);
    const GLint filter = main_batch.filter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    set_int_uniform("u_texture", 0);
    set_float_uniform("u_alpha_test", main_batch.alpha_test);

    // Apply blend mode
    glBlendFuncSeparate(
        main_batch.blend_src[0], main_batch.blend_dest[0], main_batch.blend_src[1], main_batch.blend_dest[1]
    );

    glDrawArrays(GL_TRIANGLES, 0, main_batch.vertex_count);
    main_batch.vertex_count = 0;
}

void set_main_color(GLfloat r, GLfloat g, GLfloat b) {
    main_batch.color[0] = r;
    main_batch.color[1] = g;
    main_batch.color[2] = b;
}

void set_main_alpha(GLfloat a) {
    main_batch.color[3] = a;
}

void set_main_texture(struct Texture* texture) {
    set_main_texture_direct(texture == NULL ? blank_texture : texture->texture);
}

void set_main_texture_direct(GLuint texture) {
    if (main_batch.texture != texture) {
        submit_main_batch();
        main_batch.texture = texture;
    }
}

void main_vertex(GLfloat x, GLfloat y, GLfloat z, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat u, GLfloat v) {
    if (main_batch.vertex_count >= main_batch.vertex_capacity) {
        submit_main_batch();
        main_batch.vertex_capacity *= 2;
        INFO("Reallocated main batch VBO to %u vertices", main_batch.vertex_capacity);
        lame_realloc(&main_batch.vertices, main_batch.vertex_capacity * sizeof(struct MainVertex));
        glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct MainVertex) * main_batch.vertex_capacity, NULL, GL_DYNAMIC_DRAW);
    }

    main_batch.vertices[main_batch.vertex_count++] = (struct MainVertex
    ){x, y, z, main_batch.color[0] * r, main_batch.color[1] * g, main_batch.color[2] * b, main_batch.color[3] * a,
      u, v};
}

void main_surface(struct Surface* surface, GLfloat x, GLfloat y, GLfloat z) {
    if (surface->texture[SURFACE_COLOR_TEXTURE] == 0)
        return;
    set_main_texture_direct(surface->texture[SURFACE_COLOR_TEXTURE]);

    GLfloat x1 = x;
    GLfloat y1 = y;
    GLfloat x2 = x + surface->size[0];
    GLfloat y2 = y + surface->size[1];
    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 0);
    main_vertex(x2, y2, z, 255, 255, 255, 255, 1, 0);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 1);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 1);
    main_vertex(x1, y1, z, 255, 255, 255, 255, 0, 1);
    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 0);
}

void main_sprite(struct Texture* texture, GLfloat x, GLfloat y, GLfloat z) {
    if (texture == NULL)
        return;
    set_main_texture(texture);

    GLfloat x1 = x - texture->offset[0];
    GLfloat y1 = y - texture->offset[1];
    GLfloat x2 = x1 + texture->size[0];
    GLfloat y2 = y1 + texture->size[1];
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
    main_vertex(x2, y2, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[3]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x1, y1, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[1]);
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
}

void main_material_sprite(struct Material* material, GLfloat x, GLfloat y, GLfloat z) {
    if (material == NULL || material->textures[0] == NULL)
        return;
    main_sprite(
        hid_to_texture(
            material->textures[0][(size_t)((float)draw_time * material->texture_speed[0]) % material->num_textures[0]]
        ),
        x, y, z
    );
}

void main_string(const char* str, struct Font* font, GLfloat size, GLfloat x, GLfloat y, GLfloat z) {
    if (font == NULL)
        font = default_font;
    set_main_texture(hid_to_texture(font->texture));

    GLfloat scale = size / font->size;
    GLfloat cx = x, cy = y;
    size_t bytes = SDL_strlen(str);
    while (bytes > 0) {
        size_t gid = SDL_StepUTF8(&str, &bytes);

        // Special/invalid characters
        if (gid == '\r')
            continue;
        if (gid == '\n') {
            cx = x;
            cy += size;
            continue;
        }
        if (SDL_isspace(gid))
            gid = ' ';
        if (gid >= font->num_glyphs)
            continue;

        // Valid glyph
        struct Glyph* glyph = font->glyphs[gid];
        if (glyph == NULL)
            continue;

        GLfloat x1 = cx - (glyph->offset[0] * scale);
        GLfloat y1 = cy - (glyph->offset[1] * scale);
        GLfloat x2 = x1 + (glyph->size[0] * scale);
        GLfloat y2 = y1 + (glyph->size[1] * scale);
        main_vertex(x1, y2, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[3]);
        main_vertex(x2, y2, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[3]);
        main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
        main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
        main_vertex(x1, y1, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[1]);
        main_vertex(x1, y2, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[3]);

        cx += glyph->advance * scale;
    }
}

void main_string_wrap(
    const char* str, struct Font* font, GLfloat size, GLfloat width, GLfloat x, GLfloat y, GLfloat z
) {
    if (font == NULL)
        font = default_font;
    set_main_texture(hid_to_texture(font->texture));

    GLfloat scale = size / font->size;
    GLfloat cx = x, cy = y;

    // https://github.com/raysan5/raylib/blob/master/examples/text/text_rectangle_bounds.c
    size_t bytes = SDL_strlen(str);
    bool measure = true;
    int start_pos = -1, end_pos = -1;

    for (int i = 0; i < bytes; i++) {
        const char* adv = &str[i];
        size_t advbytes = bytes - i;
        size_t last_advbytes = advbytes;

        size_t gid = SDL_StepUTF8(&adv, &advbytes);
        bool space = SDL_isspace(gid);
        struct Glyph* glyph = (gid >= font->num_glyphs || gid == '\n') ? NULL : font->glyphs[space ? (gid = ' ') : gid];
        GLfloat gwidth = glyph == NULL ? 0 : (glyph->advance * scale);

        i += (last_advbytes - advbytes) - 1;

        if (measure) {
            if (space)
                end_pos = i;

            if (((cx + gwidth) - x) > width) {
                if (end_pos <= 0)
                    end_pos = i;
                if (i == end_pos)
                    end_pos -= 1;
                if ((start_pos + 1) == end_pos)
                    end_pos = i - 1;
                measure = false;
            } else if ((i + 1) == bytes) {
                end_pos = i;
                measure = false;
            } else if (gid == '\n') {
                measure = false;
            }

            if (!measure) {
                cx = x;
                i = start_pos;
                gwidth = 0;
            }
        } else {
            if (glyph != NULL) {
                GLfloat x1 = cx - (glyph->offset[0] * scale);
                GLfloat y1 = cy - (glyph->offset[1] * scale);
                GLfloat x2 = x1 + (glyph->size[0] * scale);
                GLfloat y2 = y1 + (glyph->size[1] * scale);
                main_vertex(x1, y2, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[3]);
                main_vertex(x2, y2, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[3]);
                main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
                main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
                main_vertex(x1, y1, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[1]);
                main_vertex(x1, y2, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[3]);
            }

            if (i == end_pos) {
                cx = x;
                cy += size;
                start_pos = end_pos;
                end_pos = 0;
                measure = true;
            }
        }

        if (cx > x || !space)
            cx += gwidth;
    }
}

// World
void submit_world_batch() {}

struct ActorCamera* get_active_camera() {
    return active_camera;
}

void set_active_camera(struct ActorCamera* camera) {
    active_camera = camera;
}

struct Surface* render_camera(struct ActorCamera* camera, uint16_t width, uint16_t height) {
    if (camera->surface == NULL) {
        camera->surface = create_surface(true, width, height, true, true);
        camera->surface_ref = create_ref();
        INFO("Validating camera in \"%s\"", camera->actor->type->name);
    } else {
        resize_surface(camera->surface, width, height);
    }

    set_surface(camera->surface);
    clear_color(0.5, 0.5, 0.5, 1);
    clear_depth(0);
    clear_stencil(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    main_string("HI", NULL, 16, 0, 0, 0);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    pop_surface();

    return camera->surface;
}

// Fonts
GLfloat string_width(const char* str, struct Font* font, GLfloat size) {
    if (font == NULL)
        font = default_font;

    GLfloat scale = size / font->size;
    GLfloat cx = 0, cy = 0;
    GLfloat width = 0;
    size_t bytes = SDL_strlen(str);
    while (bytes > 0) {
        size_t gid = SDL_StepUTF8(&str, &bytes);

        // Special/invalid characters
        if (gid == '\r')
            continue;
        if (gid == '\n') {
            cx = 0;
            cy += size;
            continue;
        }
        if (SDL_isspace(gid))
            gid = ' ';
        if (gid >= font->num_glyphs)
            continue;

        // Valid glyph
        struct Glyph* glyph = font->glyphs[gid];
        if (glyph == NULL)
            continue;

        cx += glyph->advance * scale;
        if (width < cx)
            width = cx;
    }

    return width;
}

GLfloat string_height(const char* str, GLfloat size) {
    GLfloat height = size;
    size_t bytes = SDL_strlen(str);
    while (bytes > 0)
        if (SDL_StepUTF8(&str, &bytes) == '\n')
            height += size;

    return height;
}

// Surfaces
struct Surface* create_surface(bool external, uint16_t width, uint16_t height, bool color, bool depth) {
    struct Surface* surface =
        external ? userdata_alloc("surface", sizeof(struct Surface)) : lame_alloc(sizeof(struct Surface));

    surface->active = false;
    surface->stack = NULL;

    surface->enabled[SURFACE_COLOR_TEXTURE] = color;
    surface->enabled[SURFACE_DEPTH_TEXTURE] = depth;
    surface->fbo = 0;
    surface->texture[SURFACE_COLOR_TEXTURE] = surface->texture[SURFACE_DEPTH_TEXTURE] = 0;
    surface->size[0] = width;
    surface->size[1] = height;

    return surface;
}

void validate_surface(struct Surface* surface) {
    if (surface->fbo != 0)
        return;
    glGenFramebuffers(1, &surface->fbo);

    if (surface->enabled[SURFACE_COLOR_TEXTURE]) {
        if (surface->texture[SURFACE_COLOR_TEXTURE] == 0)
            glGenTextures(1, &surface->texture[SURFACE_COLOR_TEXTURE]);
        glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);
        glBindTexture(GL_TEXTURE_2D, surface->texture[SURFACE_COLOR_TEXTURE]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->size[0], surface->size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, surface->texture[SURFACE_COLOR_TEXTURE], 0
        );
    } else if (surface->texture[SURFACE_COLOR_TEXTURE] != 0) {
        glDeleteTextures(1, &surface->texture[SURFACE_COLOR_TEXTURE]);
        surface->texture[SURFACE_COLOR_TEXTURE] = 0;
    }

    if (surface->enabled[SURFACE_DEPTH_TEXTURE]) {
        if (surface->texture[SURFACE_DEPTH_TEXTURE] == 0)
            glGenTextures(1, &surface->texture[SURFACE_DEPTH_TEXTURE]);
        glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);
        glBindTexture(GL_TEXTURE_2D, surface->texture[SURFACE_DEPTH_TEXTURE]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, surface->size[0], surface->size[1], 0, GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8, NULL
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, surface->texture[SURFACE_DEPTH_TEXTURE], 0
        );
    } else if (surface->texture[SURFACE_DEPTH_TEXTURE] != 0) {
        glDeleteTextures(1, &surface->texture[SURFACE_DEPTH_TEXTURE]);
        surface->texture[SURFACE_DEPTH_TEXTURE] = 0;
    }
}

void dispose_surface(struct Surface* surface) {
    if (current_surface == surface)
        pop_surface();

    if (surface->fbo != 0) {
        glDeleteFramebuffers(1, &surface->fbo);
        surface->fbo = 0;
    }
    if (surface->texture[SURFACE_COLOR_TEXTURE] != 0) {
        glDeleteTextures(1, &surface->texture[SURFACE_COLOR_TEXTURE]);
        surface->texture[SURFACE_COLOR_TEXTURE] = 0;
    }
    if (surface->texture[SURFACE_DEPTH_TEXTURE] != 0) {
        glDeleteTextures(1, &surface->texture[SURFACE_DEPTH_TEXTURE]);
        surface->texture[SURFACE_DEPTH_TEXTURE] = 0;
    }
}

void destroy_surface(struct Surface* surface) {
    dispose_surface(surface);
    lame_free(&surface);
}

void set_surface(struct Surface* surface) {
    if (current_surface != surface) {
        submit_batch();
        if (current_surface != NULL)
            current_surface->active = false;
        if (surface != NULL) {
            surface->active = true;
            surface->stack = current_surface;
            validate_surface(surface);
            glViewport(0, 0, surface->size[0], surface->size[1]);
            glm_ortho(0, surface->size[0], surface->size[1], 0, -1000, 1000, projection_matrix);
        } else {
            glViewport(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT);
            glm_ortho(0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 0, -1000, 1000, projection_matrix);
        }
        glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
        glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);
        glBindFramebuffer(GL_FRAMEBUFFER, surface == NULL ? 0 : surface->fbo);
        current_surface = surface;
    }
}

void pop_surface() {
    if (current_surface == NULL)
        return;
    set_surface(current_surface->stack);
}

void resize_surface(struct Surface* surface, uint16_t width, uint16_t height) {
    if (surface->size[0] == width && surface->size[1] == height)
        return;
    dispose_surface(surface);
    surface->size[0] = width;
    surface->size[1] = height;
    INFO("Resized surface %u to %ux%u px", surface, width, height);
}

void clear_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clear_depth(GLfloat depth) {
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void clear_stencil(GLint stencil) {
    glClearStencil(stencil);
    glClear(GL_STENCIL_BUFFER_BIT);
}
