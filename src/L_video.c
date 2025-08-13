#include <SDL3/SDL_timer.h>

#include "L_actor.h"
#include "L_config.h"
#include "L_internal.h"
#include "L_localize.h"
#include "L_log.h"
#include "L_memory.h"
#include "L_player.h"
#include "L_ui.h"
#include "L_video.h"

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

static struct Display display = {DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 0};
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
static struct Shader* sky_shader = NULL;
static struct Shader* current_shader = NULL;
static struct Font* default_font = NULL;
static struct Surface* current_surface = NULL;

static mat4 forward_axis = GLM_MAT4_IDENTITY_INIT;
static mat4 up_axis = GLM_MAT4_IDENTITY_INIT;

static mat4 model_matrix = GLM_MAT4_IDENTITY_INIT;
static mat4 view_matrix = GLM_MAT4_IDENTITY_INIT;
static mat4 projection_matrix = GLM_MAT4_IDENTITY_INIT;
static mat4 mvp_matrix = GLM_MAT4_IDENTITY_INIT;

void video_init(bool bypass_shader) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Window
    window = SDL_CreateWindow(
        SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING), display.width, display.height, SDL_WINDOW_OPENGL
    );
    if (window == NULL)
        FATAL("Window fail: %s", SDL_GetError());

    // OpenGL
    gpu = SDL_GL_CreateContext(window);
    if (gpu == NULL || !SDL_GL_MakeCurrent(window, gpu))
        FATAL("GPU fail: %s", SDL_GetError());
    SDL_GL_SetSwapInterval(0);

    update_display();

    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0)
        FATAL("Failed to load OpenGL functions");
    INFO("GLAD version: %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    if (!GLAD_GL_VERSION_3_3)
        FATAL("Unsupported OpenGL version\nAt least OpenGL 3.3 with framebuffer and shader support is required.");
    CHECK_GL_EXTENSION(GLAD_GL_EXT_framebuffer_object);
    if (bypass_shader) {
        WARN("Bypassing shader support");
    } else {
        CHECK_GL_EXTENSION(GLAD_GL_ARB_shader_objects);
        CHECK_GL_EXTENSION(GLAD_GL_ARB_vertex_shader);
        CHECK_GL_EXTENSION(GLAD_GL_ARB_fragment_shader);
        CHECK_GL_EXTENSION(GLAD_GL_ARB_vertex_program);
        CHECK_GL_EXTENSION(GLAD_GL_ARB_fragment_program);
    }

    INFO("OpenGL vendor: %s", glGetString(GL_VENDOR));
    INFO("OpenGL version: %s", glGetString(GL_VERSION));
    INFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
    INFO("OpenGL shading language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Blank texture
    glGenTextures(1, &blank_texture);
    glBindTexture(GL_TEXTURE_2D, blank_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const uint8_t[]){255, 255, 255, 255});

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
    glBufferData(
        GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct MainVertex) * main_batch.vertex_capacity), NULL, GL_DYNAMIC_DRAW
    );

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
    glVertexArrayAttribFormat(world_batch.vao, VATT_BONE_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);
    glEnableVertexArrayAttrib(world_batch.vao, VATT_BONE_WEIGHT);
    glVertexArrayAttribFormat(world_batch.vao, VATT_BONE_WEIGHT, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);

    world_batch.vertex_count = 0;
    world_batch.vertex_capacity = 3;
    world_batch.vertices = lame_alloc(world_batch.vertex_capacity * sizeof(struct WorldVertex));

    glGenBuffers(1, &world_batch.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, world_batch.vbo);
    glBufferData(
        GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct WorldVertex) * world_batch.vertex_capacity), NULL, GL_DYNAMIC_DRAW
    );

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
        VATT_BONE_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(struct WorldVertex),
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
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Build matrices
    glm_translate_x(forward_axis, 1);
    glm_translate_z(up_axis, -1);

    glm_ortho(0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 0, -1000, 1000, projection_matrix);
    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);

    // Shaders
    if (default_shaders[RT_MAIN] == NULL) {
        default_shaders[RT_MAIN] = fetch_shader("main");
        if (default_shaders[RT_MAIN] == NULL)
            FATAL("Main shader \"main\" not found");
    }
    if (default_shaders[RT_WORLD] == NULL) {
        default_shaders[RT_WORLD] = fetch_shader("world");
        if (default_shaders[RT_WORLD] == NULL)
            FATAL("World shader \"world\" not found");
    }
    if (sky_shader == NULL) {
        sky_shader = fetch_shader("sky");
        if (sky_shader == NULL)
            FATAL("Sky shader \"sky\" not found");
    }

    // Fonts
    default_font = fetch_font("main");
    if (default_font == NULL)
        FATAL("Main font \"main\" not found");
    // GROSS HACK: Font texture has to be manually made transient
    default_font->transient = default_font->texture->transient = true;

    INFO("Opened");
}

void video_update() {
    draw_time = SDL_GetTicks();
    if (framerate > 0) {
        cap_wait += (float)(draw_time - last_cap_time) * ((float)framerate / 1000.0f);
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
                if (player->actor != NULL) {
                    camera = player->actor->camera;
                    if (camera != NULL)
                        break;
                }
                player = player->previous_active;
            }
        }
        if (camera != NULL) {
            struct Surface* surface = render_camera(camera, display.width, display.height, true, NULL, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, surface->fbo);
            glBlitFramebuffer(
                0, 0, surface->size[0], surface->size[1], 0, display.height, display.width, 0, GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
        }

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

    submit_main_batch();

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
    if (display.width == width && display.height == height && display.fullscreen == fullscreen &&
        display.vsync == vsync)
        return;

    display.width = width <= 0 ? DEFAULT_DISPLAY_WIDTH : width;
    display.height = height <= 0 ? DEFAULT_DISPLAY_HEIGHT : height;
    display.fullscreen = SDL_min(fullscreen, FSM_EXCLUSIVE_FULLSCREEN);
    display.vsync = vsync;

    update_display();

    INFO(
        "Display set to %dx%d (%dx%d), mode %d, vsync %d", display.width, display.height, get_int_cvar("vid_width"),
        get_int_cvar("vid_height"), display.fullscreen, display.vsync
    );
}

void update_display() {
    if (window == NULL || gpu == NULL)
        return;
    // MEMORY LEAK: In exclusive fullscreen, Windows will leak ~220 bytes every
    //              time you tab out and back in.
    //              https://github.com/libsdl-org/SDL/issues/13233
    // Pattern: <C D D           > 43 00 44 00 44 00 00 00 00 00 00 00 00 00 00 00
    // Severity: High. This part is commented out until SDL 3.4.0 releases.

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
    SDL_SetWindowSize(window, display.width, display.height);
    SDL_GL_SetSwapInterval(display.vsync);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RestoreWindow(window);
    SDL_SyncWindow(window);
}

uint16_t get_framerate() {
    return framerate;
}

void set_framerate(uint16_t fps) {
    if (framerate == fps)
        return;
    framerate = fps;
    if (framerate > 0)
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

void set_uint_uniform(const char* name, const GLuint value) {
    glUniform1ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_uvec2_uniform(const char* name, const GLuint value[2]) {
    glUniform2ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1]);
}

void set_uvec3_uniform(const char* name, const GLuint value[3]) {
    glUniform3ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[3]);
}

void set_uvec4_uniform(const char* name, const GLuint value[4]) {
    glUniform4ui(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[2], value[3]
    );
}

void set_int_uniform(const char* name, const GLint value) {
    glUniform1i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_ivec2_uniform(const char* name, const GLint value[2]) {
    glUniform2i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1]);
}

void set_ivec3_uniform(const char* name, const GLint value[3]) {
    glUniform3i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[2]);
}

void set_ivec4_uniform(const char* name, const GLint value[4]) {
    glUniform4i(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[2], value[3]
    );
}

void set_float_uniform(const char* name, const GLfloat value) {
    glUniform1f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

void set_vec2_uniform(const char* name, const GLfloat value[2]) {
    glUniform2f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1]);
}

void set_vec3_uniform(const char* name, const GLfloat value[3]) {
    glUniform3f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[2]);
}

void set_vec4_uniform(const char* name, const GLfloat value[4]) {
    glUniform4f(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value[0], value[1], value[2], value[3]
    );
}

void set_mat2_uniform(const char* name, mat2 matrix) {
    glUniformMatrix2fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

void set_mat3_uniform(const char* name, mat3 matrix) {
    glUniformMatrix3fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

void set_mat4_uniform(const char* name, mat4 matrix) {
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
    set_mat4_uniform("u_model_matrix", model_matrix);
    set_mat4_uniform("u_view_matrix", view_matrix);
    set_mat4_uniform("u_projection_matrix", projection_matrix);
    set_mat4_uniform("u_mvp_matrix", mvp_matrix);

    glBindVertexArray(main_batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0, (GLsizeiptr)(sizeof(struct MainVertex) * main_batch.vertex_count), main_batch.vertices
    );

    // Apply stencil
    set_vec4_uniform("u_stencil", main_batch.stencil);

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

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)main_batch.vertex_count);
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

        const size_t new_size = main_batch.vertex_capacity * 2;
        if (new_size < main_batch.vertex_capacity)
            FATAL("Capacity overflow in main batch VBO");
        lame_realloc(&main_batch.vertices, new_size * sizeof(struct MainVertex));
        main_batch.vertex_capacity = new_size;

        glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
        glBufferData(
            GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct MainVertex) * main_batch.vertex_capacity), NULL, GL_DYNAMIC_DRAW
        );
    }

    main_batch.vertices[main_batch.vertex_count++] = (struct MainVertex){x,
                                                                         y,
                                                                         z,
                                                                         (GLubyte)(main_batch.color[0] * (GLfloat)r),
                                                                         (GLubyte)(main_batch.color[1] * (GLfloat)g),
                                                                         (GLubyte)(main_batch.color[2] * (GLfloat)b),
                                                                         (GLubyte)(main_batch.color[3] * (GLfloat)a),
                                                                         u,
                                                                         v};
}

void main_rectangle(
    GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat z, GLubyte r, GLubyte g, GLubyte b, GLubyte a
) {
    set_main_texture_direct(blank_texture);
    main_vertex(x1, y2, z, r, g, b, a, 0, 1);
    main_vertex(x1, y1, z, r, g, b, a, 0, 0);
    main_vertex(x2, y1, z, r, g, b, a, 1, 0);
    main_vertex(x2, y1, z, r, g, b, a, 1, 0);
    main_vertex(x2, y2, z, r, g, b, a, 1, 1);
    main_vertex(x1, y2, z, r, g, b, a, 0, 1);
}

void main_surface(struct Surface* surface, GLfloat x, GLfloat y, GLfloat z) {
    if (surface->texture[SURFACE_COLOR_TEXTURE] == 0)
        return;
    set_main_texture_direct(surface->texture[SURFACE_COLOR_TEXTURE]);

    GLfloat x1 = x;
    GLfloat y1 = y;
    GLfloat x2 = x + (GLfloat)surface->size[0];
    GLfloat y2 = y + (GLfloat)surface->size[1];
    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 1);
    main_vertex(x1, y1, z, 255, 255, 255, 255, 0, 0);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 0);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 0);
    main_vertex(x2, y2, z, 255, 255, 255, 255, 1, 1);
    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 1);
}

void main_surface_rectangle(struct Surface* surface, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat z) {
    if (surface->texture[SURFACE_COLOR_TEXTURE] == 0)
        return;
    set_main_texture_direct(surface->texture[SURFACE_COLOR_TEXTURE]);

    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 1);
    main_vertex(x1, y1, z, 255, 255, 255, 255, 0, 0);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 0);
    main_vertex(x2, y1, z, 255, 255, 255, 255, 1, 0);
    main_vertex(x2, y2, z, 255, 255, 255, 255, 1, 1);
    main_vertex(x1, y2, z, 255, 255, 255, 255, 0, 1);
}

void main_sprite(struct Texture* texture, GLfloat x, GLfloat y, GLfloat z) {
    if (texture == NULL)
        return;
    set_main_texture(texture);

    GLfloat x1 = x - (GLfloat)texture->offset[0];
    GLfloat y1 = y - (GLfloat)texture->offset[1];
    GLfloat x2 = x1 + (GLfloat)texture->size[0];
    GLfloat y2 = y1 + (GLfloat)texture->size[1];
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
    main_vertex(x1, y1, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[1]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x2, y2, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[3]);
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
}

void main_material_sprite(struct Material* material, GLfloat x, GLfloat y, GLfloat z) {
    if (material == NULL || material->textures[0] == NULL)
        return;
    main_sprite(
        material->textures[0][(size_t)((float)draw_time * material->texture_speed[0]) % material->num_textures[0]], x,
        y, z
    );
}

void main_string(const char* str, struct Font* font, GLfloat size, GLfloat x, GLfloat y, GLfloat z) {
    if (font == NULL)
        font = default_font;
    set_main_texture(font->texture);

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
        if (SDL_isspace((int)gid))
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
        main_vertex(x1, y1, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[1]);
        main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
        main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
        main_vertex(x2, y2, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[3]);
        main_vertex(x1, y2, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[3]);

        cx += glyph->advance * scale;
    }
}

void main_string_wrap(
    const char* str, struct Font* font, GLfloat size, GLfloat width, GLfloat x, GLfloat y, GLfloat z
) {
    if (font == NULL)
        font = default_font;
    set_main_texture(font->texture);

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
        bool space = SDL_isspace((int)gid);
        struct Glyph* glyph = (gid >= font->num_glyphs || gid == '\n') ? NULL : font->glyphs[space ? (gid = ' ') : gid];
        GLfloat gwidth = glyph == NULL ? 0 : (glyph->advance * scale);

        i += (int)(last_advbytes - advbytes) - 1;

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
                main_vertex(x1, y1, z, 255, 255, 255, 255, glyph->uvs[0], glyph->uvs[1]);
                main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
                main_vertex(x2, y1, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[1]);
                main_vertex(x2, y2, z, 255, 255, 255, 255, glyph->uvs[2], glyph->uvs[3]);
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
void submit_world_batch() {
    if (world_batch.vertex_count <= 0)
        return;

    // Apply matrices
    set_mat4_uniform("u_model_matrix", model_matrix);
    set_mat4_uniform("u_view_matrix", view_matrix);
    set_mat4_uniform("u_projection_matrix", projection_matrix);
    set_mat4_uniform("u_mvp_matrix", mvp_matrix);

    glBindVertexArray(world_batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, world_batch.vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0, (GLsizeiptr)(sizeof(struct WorldVertex) * world_batch.vertex_count), world_batch.vertices
    );

    set_int_uniform("u_animated", 0);
    set_vec4_uniform("u_color", world_batch.color);
    set_vec4_uniform("u_stencil", world_batch.stencil);

    // Apply texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world_batch.texture);
    const GLint filter = world_batch.filter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    set_int_uniform("u_texture", 0);
    set_int_uniform("u_has_blend_texture", 0);
    set_int_uniform("u_blend_texture", 1);
    set_float_uniform("u_alpha_test", world_batch.alpha_test);
    set_vec2_uniform("u_scroll", (GLfloat[2]){0});
    set_vec3_uniform("u_material_wind", (GLfloat[3]){0});
    set_float_uniform("u_bright", world_batch.bright);
    set_int_uniform("u_half_lambert", 0);
    set_float_uniform("u_cel", 0);
    set_vec4_uniform("u_specular", (GLfloat[4]){0, 1, 0, 1});

    // Apply blend mode
    glBlendFuncSeparate(
        world_batch.blend_src[0], world_batch.blend_dest[0], world_batch.blend_src[1], world_batch.blend_dest[1]
    );

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)world_batch.vertex_count);
    world_batch.vertex_count = 0;
}

void set_world_color(GLfloat r, GLfloat g, GLfloat b) {
    world_batch.color[0] = r;
    world_batch.color[1] = g;
    world_batch.color[2] = b;
}

void set_world_alpha(GLfloat a) {
    world_batch.color[3] = a;
}

void set_world_texture(struct Texture* texture) {
    set_world_texture_direct(texture == NULL ? blank_texture : texture->texture);
}

void set_world_texture_direct(GLuint texture) {
    if (world_batch.texture != texture) {
        submit_world_batch();
        world_batch.texture = texture;
    }
}

void world_vertex(
    GLfloat x, GLfloat y, GLfloat z, GLfloat nx, GLfloat ny, GLfloat nz, GLubyte r, GLubyte g, GLubyte b, GLubyte a,
    GLfloat u, GLfloat v
) {
    if (world_batch.vertex_count >= world_batch.vertex_capacity) {
        submit_world_batch();

        const size_t new_size = world_batch.vertex_capacity * 2;
        if (new_size < world_batch.vertex_capacity)
            FATAL("Capacity overflow in world batch VBO");
        lame_realloc(&world_batch.vertices, new_size * sizeof(struct WorldVertex));
        world_batch.vertex_capacity = new_size;

        glBindBuffer(GL_ARRAY_BUFFER, world_batch.vbo);
        glBufferData(
            GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct WorldVertex) * world_batch.vertex_capacity), NULL,
            GL_DYNAMIC_DRAW
        );
    }

    world_batch.vertices[world_batch.vertex_count++] =
        (struct WorldVertex){x,
                             y,
                             z,
                             nx,
                             ny,
                             nz,
                             (GLubyte)(world_batch.color[0] * (GLfloat)r),
                             (GLubyte)(world_batch.color[1] * (GLfloat)g),
                             (GLubyte)(world_batch.color[2] * (GLfloat)b),
                             (GLubyte)(world_batch.color[3] * (GLfloat)a),
                             u,
                             v,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0};
}

struct ActorCamera* get_active_camera() {
    return active_camera;
}

void set_active_camera(struct ActorCamera* camera) {
    active_camera = camera;
}

struct Surface* render_camera(
    struct ActorCamera* camera, uint16_t width, uint16_t height, bool draw_screen, struct Shader* world_shader,
    int listener
) {
    if (camera->surface == NULL) {
        camera->surface = create_surface(true, width, height, true, true);
        camera->surface_ref = create_ref();
    } else {
        resize_surface(camera->surface, width, height);
    }

    set_surface(camera->surface);
    clear_depth(1);
    clear_stencil(0);

    // Build matrices
    static mat4 lookie;
    glm_mat4_identity(lookie);
    if (camera->flags & CF_THIRD_PERSON)
        glm_translate_x(lookie, -camera->draw_range[1]);
    glm_spin(lookie, glm_rad(camera->draw_angle[1][0]), GLM_ZUP);
    glm_spin(lookie, glm_rad(camera->draw_angle[1][1]), GLM_YUP);
    glm_spin(lookie, glm_rad(camera->draw_angle[1][2]), GLM_XUP);

    static mat4 forward_matrix, up_matrix;
    static vec3 forward_vector, up_vector;
    glm_mat4_mul(lookie, forward_axis, forward_matrix);
    glm_vec3_copy(forward_matrix[3], forward_vector);
    glm_mat4_mul(lookie, up_axis, up_matrix);
    glm_vec3_copy(up_matrix[3], up_vector);

    static vec3 look_from, look_to;
    glm_vec3_copy(camera->draw_pos[1], look_from);
    if (listener >= 0)
        update_listener(listener, look_from, GLM_VEC3_ZERO, forward_vector, up_vector);
    glm_vec3_add(forward_vector, look_from, look_to);

    glm_lookat(look_from, look_to, up_vector, view_matrix);
    if (camera->flags & CF_ORTHOGONAL)
        glm_ortho(0, width, height, 0, 1, 32000, projection_matrix);
    else
        glm_perspective(-glm_rad(camera->draw_fov[1]), -(float)width / (float)height, 1, 32000, projection_matrix);

    // Render room
    set_render_stage(RT_WORLD);

    struct Room* room = camera->actor->room;
    const float u_time = (float)draw_time / 1000.0f;

    struct Actor* sky = room->sky;
    if (sky != NULL) {
        static mat4 sky_view;
        glm_lookat(GLM_VEC3_ZERO, forward_vector, up_vector, sky_view);
        glm_mat4_mul(projection_matrix, sky_view, mvp_matrix);
        set_shader(sky_shader);
        set_mat4_uniform("u_mvp_matrix", mvp_matrix);
        set_float_uniform("u_time", u_time);

        if (sky->model != NULL)
            submit_model_instance(sky->model);
        else
            clear_color(0, 0, 0, 1);
        if (sky->type->draw != LUA_NOREF)
            execute_ref_in(sky->type->draw, sky->userdata, sky->type->name);

        submit_world_batch();
    } else {
        clear_color(0.5f, 0.5f, 0.5f, 1);
    }

    glm_mat4_identity(model_matrix);
    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);

    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    set_shader(world_shader);
    set_float_uniform("u_time", u_time);
    set_vec4_uniform("u_ambient", room->ambient);
    glUniform1fv(
        (GLint)(SDL_GetNumberProperty(current_shader->uniforms, "u_lights[0]", -1)),
        MAX_ROOM_LIGHTS * (sizeof(struct RoomLight) / sizeof(GLfloat)), (const GLfloat*)room->lights
    );
    set_vec2_uniform("u_fog_distance", room->fog_distance);
    set_vec4_uniform("u_fog_color", room->fog_color);
    set_vec4_uniform("u_wind", room->wind);

    if (room->model != NULL)
        draw_model_instance(room->model);

    struct Actor* actor = room->actors;
    while (actor != NULL) {
        if (actor->flags & AF_VISIBLE && (camera != actor->camera || (camera->flags & CF_THIRD_PERSON))) {
            static vec3 center;
            glm_vec3_copy(actor->draw_pos[1], center);
            center[2] -= actor->collision_size[1] * 0.5f;

            const float distance = glm_vec3_distance(camera->draw_pos[1], center);
            if (distance > actor->cull_draw[0] && distance < actor->cull_draw[1]) {
                if (actor->model != NULL)
                    draw_model_instance(actor->model);
                if (actor->type->draw != LUA_NOREF)
                    execute_ref_in_child(actor->type->draw, actor->userdata, camera->userdata, actor->type->name);
            }
        }
        actor = actor->previous_neighbor;
    }

    submit_world_batch();

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    set_render_stage(RT_MAIN);
    set_shader(NULL);

    // Draw in surface screen space
    glm_mat4_identity(model_matrix);
    glm_mat4_identity(view_matrix);
    glm_ortho(0, width, 0, height, -1000, 1000, projection_matrix);
    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);

    actor = room->actors;
    while (actor != NULL) {
        if (actor->flags & AF_VISIBLE && actor->type->draw_screen != LUA_NOREF)
            execute_ref_in_child(actor->type->draw_screen, actor->userdata, camera->userdata, actor->type->name);
        actor = actor->previous_neighbor;
    }

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
        if (SDL_isspace((int)gid))
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
        external ? userdata_alloc_clean("surface", sizeof(struct Surface)) : lame_alloc_clean(sizeof(struct Surface));

    surface->enabled[SURFACE_COLOR_TEXTURE] = color;
    surface->enabled[SURFACE_DEPTH_TEXTURE] = depth;
    surface->size[0] = width;
    surface->size[1] = height;

    glm_mat4_identity(surface->model_matrix);
    glm_mat4_identity(surface->view_matrix);
    glm_ortho(0, width, 0, height, -1000, 1000, surface->projection_matrix);
    glm_mat4_mul(surface->view_matrix, surface->model_matrix, surface->mvp_matrix);
    glm_mat4_mul(surface->projection_matrix, surface->mvp_matrix, surface->mvp_matrix);

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
        if (current_surface != NULL) {
            current_surface->active = false;
            glm_mat4_copy(model_matrix, current_surface->model_matrix);
            glm_mat4_copy(view_matrix, current_surface->view_matrix);
            glm_mat4_copy(projection_matrix, current_surface->projection_matrix);
            glm_mat4_copy(mvp_matrix, current_surface->mvp_matrix);
        }
        if (surface != NULL) {
            if (surface->active)
                FATAL("Setting an active surface?");
            surface->active = true;
            if (current_surface != NULL && current_surface->stack == surface)
                current_surface->stack = NULL;
            else
                surface->stack = current_surface;
            validate_surface(surface);
            glViewport(0, 0, surface->size[0], surface->size[1]);
            glm_mat4_copy(surface->model_matrix, model_matrix);
            glm_mat4_copy(surface->view_matrix, view_matrix);
            glm_mat4_copy(surface->projection_matrix, projection_matrix);
            glm_mat4_copy(surface->mvp_matrix, mvp_matrix);
        } else {
            glViewport(0, 0, display.width, display.height);
            glm_mat4_identity(model_matrix);
            glm_mat4_identity(view_matrix);
            glm_ortho(0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 0, -1000, 1000, projection_matrix);
            glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
            glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);
        }
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
    if (surface->active)
        FATAL("Resizing an active surface?");
    if (surface->size[0] == width && surface->size[1] == height)
        return;
    dispose_surface(surface);
    surface->size[0] = width;
    surface->size[1] = height;

    glm_ortho(0, width, 0, height, -1000, 1000, surface->projection_matrix);
    glm_mat4_mul(surface->view_matrix, surface->model_matrix, surface->mvp_matrix);
    glm_mat4_mul(surface->projection_matrix, surface->mvp_matrix, surface->mvp_matrix);
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

// Model Instance
struct ModelInstance* create_model_instance(struct Model* model) {
    struct ModelInstance* inst = lame_alloc_clean(sizeof(struct ModelInstance));

    inst->model = model;
    inst->userdata = create_pointer_ref("model_instance", inst);
    glm_vec3_one(inst->scale);
    glm_vec3_one(inst->draw_scale[0]);
    glm_vec3_one(inst->draw_scale[1]);
    glm_vec4_one(inst->color);

    inst->hidden = lame_alloc_clean(model->num_submodels * sizeof(bool));
    inst->override_materials = (struct Material**)lame_alloc_clean(model->num_materials * sizeof(struct Material*));
    inst->override_textures = lame_alloc_clean(model->num_materials * sizeof(GLuint));

    inst->frame_speed = 1;

    return inst;
}

void destroy_model_instance(struct ModelInstance* inst) {
    unreference_pointer(&(inst->userdata));

    FREE_POINTER(inst->hidden);
    FREE_POINTER(inst->override_materials);
    FREE_POINTER(inst->override_textures);

    FREE_POINTER(inst->translations);
    FREE_POINTER(inst->rotations);
    FREE_POINTER(inst->transforms);
    FREE_POINTER(inst->sample);
    FREE_POINTER(inst->draw_sample[0]);
    FREE_POINTER(inst->draw_sample[1]);

    lame_free(&inst);
}

static void animate_model_instance(struct ModelInstance* inst, bool snap) {
    const struct Animation* animation = inst->animation;
    if (inst->animation == NULL)
        return;

    static DualQuaternion transframe[MAX_BONES] = {0};
    float frm = SDL_fabsf(inst->frame);

    if (animation->bone_frames != NULL) {
        // Copy sample from bone-space frame
        const DualQuaternion* frame_a = animation->bone_frames[(size_t)SDL_fmodf(frm, (float)animation->num_frames)];
        const DualQuaternion* frame_b =
            animation->bone_frames[(size_t)(inst->loop ? SDL_fmodf(frm + 1, (float)animation->num_frames)
                                                       : SDL_min(frm + 1, animation->num_frames - 1))];

        const DualQuaternion* frame;
        if (frame_a == frame_b) {
            frame = frame_a;
        } else {
            float blend = frm - SDL_floorf(frm);
            for (size_t i = 0; i < animation->num_bones; i++)
                dq_lerp(frame_a[i], frame_b[i], blend, transframe[i]);
            frame = transframe;
        }

        lame_copy(inst->sample, frame, animation->num_bones * sizeof(DualQuaternion));
    } else if (animation->parent_frames != NULL) {
        // Generate a sample from parent-space frame
        // https://github.com/blueburncz/BBMOD/blob/bbmod3/BBMOD_GML/scripts/BBMOD_AnimationPlayer/BBMOD_AnimationPlayer.gml#L152
        const DualQuaternion* frame_a = animation->parent_frames[(size_t)SDL_fmodf(frm, (float)animation->num_frames)];
        const DualQuaternion* frame_b =
            animation->parent_frames[(size_t)(inst->loop ? SDL_fmodf(frm + 1, (float)animation->num_frames)
                                                         : SDL_min(frm + 1, animation->num_frames - 1))];

        const DualQuaternion* frame;
        if (frame_a == frame_b) {
            frame = frame_a;
        } else {
            float blend = frm - SDL_floorf(frm);
            for (size_t i = 0; i < animation->num_nodes; i++)
                dq_slerp(frame_a[i], frame_b[i], blend, transframe[i]);
            frame = transframe;
        }

        static const struct Node* node_stack[MAX_BONES] = {0};
        const struct Model* model = inst->model;
        node_stack[0] = model->root_node;
        size_t next = 1;

        for (size_t i = 0; i < model->num_nodes; i++) {
            if (next <= 0)
                break;
            const struct Node* node = node_stack[--next];

            // TODO: Separate skeleton from the rest of the nodes to save on
            //       iterations here.

            size_t idx = node->index;
            if (idx >= animation->num_nodes)
                continue;

            static DualQuaternion ndq;
            dq_copy(frame[idx], ndq);
            glm_quat_mul(inst->rotations[idx], ndq, ndq);
            glm_quat_add(inst->translations[idx], &ndq[4], &ndq[4]);

            if (node->parent != NULL)
                dq_mul(ndq, inst->transforms[node->parent->index], ndq);
            dq_copy(ndq, inst->transforms[idx]);

            if (node->bone)
                dq_mul(model->bone_offsets[idx], inst->transforms[idx], inst->sample[idx]);

            for (size_t j = 0; j < node->num_children; j++)
                node_stack[next++] = node->children[j];
        }
    }

    if (snap)
        lame_copy(inst->draw_sample[0], inst->sample, inst->model->num_nodes * sizeof(DualQuaternion));
}

void set_model_instance_animation(struct ModelInstance* inst, struct Animation* animation, float frame, bool loop) {
    if (animation != NULL) {
        if (inst->sample == NULL) {
            inst->translations = lame_alloc_clean(animation->num_nodes * sizeof(versor));
            inst->rotations = lame_alloc(animation->num_nodes * sizeof(versor));
            for (size_t i = 0; i < animation->num_nodes; i++)
                glm_quat_identity(inst->rotations[i]);

            inst->transforms = lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            inst->sample = lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            inst->draw_sample[0] = lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            inst->draw_sample[1] = lame_alloc(animation->num_nodes * sizeof(DualQuaternion));
            for (size_t i = 0; i < animation->num_nodes; i++) {
                dq_identity(inst->transforms[i]);
                dq_identity(inst->sample[i]);
                dq_identity(inst->draw_sample[0][i]);
            }
        } else if (inst->animation != NULL && inst->animation->num_nodes < animation->num_nodes) {
            lame_realloc(&(inst->translations), animation->num_nodes * sizeof(versor));
            lame_realloc(&(inst->rotations), animation->num_nodes * sizeof(versor));
            for (size_t i = inst->animation->num_nodes; i < animation->num_nodes; i++) {
                glm_vec4_zero(inst->translations[i]);
                glm_quat_identity(inst->rotations[i]);
            }

            lame_realloc(&(inst->transforms), animation->num_nodes * sizeof(DualQuaternion));
            lame_realloc(&(inst->sample), animation->num_nodes * sizeof(DualQuaternion));
            lame_realloc(&(inst->draw_sample[0]), animation->num_nodes * sizeof(DualQuaternion));
            lame_realloc(&(inst->draw_sample[1]), animation->num_nodes * sizeof(DualQuaternion));
            for (size_t i = inst->animation->num_nodes; i < animation->num_nodes; i++) {
                dq_identity(inst->transforms[i]);
                dq_identity(inst->sample[i]);
                dq_identity(inst->draw_sample[0][i]);
            }
        }
    }

    inst->animation = animation;
    inst->frame = frame;
    inst->loop = loop;
    if (animation != NULL)
        animate_model_instance(inst, true);
}

void translate_model_instance_node(struct ModelInstance* inst, size_t node_index, versor quat) {
    if (inst->translations != NULL)
        glm_quat_copy(quat, inst->translations[node_index]);
}

void rotate_model_instance_node(struct ModelInstance* inst, size_t node_index, versor quat) {
    if (inst->rotations != NULL)
        glm_quat_copy(quat, inst->rotations[node_index]);
}

void tick_model_instance(struct ModelInstance* inst) {
    struct Animation* animation = inst->animation;
    if (animation == NULL)
        return;
    inst->frame += animation->frame_speed * inst->frame_speed;
    if (!inst->loop && SDL_fabsf(inst->frame) >= (float)(animation->num_frames - 1))
        inst->frame = (float)(animation->num_frames - 1);
    animate_model_instance(inst, false);
}

void submit_model_instance(struct ModelInstance* inst) {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    set_int_uniform("u_texture", 0);
    set_vec4_uniform("u_color", inst->color);
    set_vec4_uniform("u_stencil", (GLfloat[]){1, 1, 1, 0});

    if (inst->model->lightmap != NULL) {
        set_int_uniform("u_has_lightmap", 1);
        set_int_uniform("u_lightmap", 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, inst->model->lightmap->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        set_int_uniform("u_has_lightmap", 0);
    }

    if (inst->animation != NULL && inst->draw_sample[1] != NULL) {
        set_int_uniform("u_animated", 1);
        glUniform4fv(
            (GLint)(SDL_GetNumberProperty(current_shader->uniforms, "u_sample[0]", -1)),
            (GLsizei)(2 * inst->model->num_bones), (const GLfloat*)(inst->draw_sample[1])
        );
    } else {
        set_int_uniform("u_animated", 0);
    }

    struct Model* model = inst->model;
    for (size_t i = 0; i < model->num_submodels; i++) {
        if (inst->hidden[i])
            continue;

        const struct Submodel* submodel = &(model->submodels[i]);

        struct Material* material = inst->override_materials[submodel->material];
        if (material == NULL) {
            material = model->materials[submodel->material];
            if (material == NULL)
                continue;
        }

        GLuint tex = inst->override_textures[submodel->material];
        if (tex == 0) {
            const struct Texture* texture =
                material->textures[0] == NULL
                    ? NULL
                    : material->textures[0][(size_t)SDL_fmodf(
                          (float)draw_time * material->texture_speed[0], (float)material->num_textures[0]
                      )];
            tex = (texture == NULL) ? blank_texture : texture->texture;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        const GLint filter = material->filter ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

        if (material->textures[1] != NULL) {
            set_int_uniform("u_has_blend_texture", 1);
            set_int_uniform("u_blend_texture", 1);
            const struct Texture* blend_texture = material->textures[1][(size_t)SDL_fmodf(
                (float)draw_time * material->texture_speed[1], (float)material->num_textures[1]
            )];
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, blend_texture == NULL ? blank_texture : blend_texture->texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        } else {
            set_int_uniform("u_has_blend_texture", 0);
        }

        set_float_uniform("u_alpha_test", material->alpha_test);
        set_vec2_uniform("u_scroll", material->scroll);
        set_vec3_uniform("u_material_wind", material->wind);
        set_float_uniform("u_bright", material->bright);
        set_int_uniform("u_half_lambert", material->half_lambert);
        set_float_uniform("u_cel", material->cel);
        set_vec4_uniform("u_specular", material->specular);

        glBindVertexArray(submodel->vao);
        glBindBuffer(GL_ARRAY_BUFFER, submodel->vbo);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0, (GLsizeiptr)(sizeof(struct WorldVertex) * submodel->num_vertices), submodel->vertices
        );
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)submodel->num_vertices);
    }
}

void draw_model_instance(struct ModelInstance* inst) {
    glm_mat4_identity(model_matrix);
    glm_scale(model_matrix, inst->draw_scale[1]);
    glm_spin(model_matrix, glm_rad(inst->draw_angle[1][0]), GLM_ZUP);
    glm_spin(model_matrix, glm_rad(inst->draw_angle[1][1]), GLM_YUP);
    glm_spin(model_matrix, glm_rad(inst->draw_angle[1][2]), GLM_XUP);
    glm_translated(model_matrix, inst->draw_pos[1]);

    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);

    set_mat4_uniform("u_model_matrix", model_matrix);
    set_mat4_uniform("u_view_matrix", view_matrix);
    set_mat4_uniform("u_projection_matrix", projection_matrix);
    set_mat4_uniform("u_mvp_matrix", mvp_matrix);

    submit_model_instance(inst);

    glm_mat4_identity(model_matrix);
    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);
}
