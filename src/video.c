#include "video.h"
#include "asset.h"
#include "log.h"
#include "mem.h"

#define DEFAULT_DISPLAY_WIDTH 640
#define DEFAULT_DISPLAY_HEIGHT 480

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

static struct Display display = {-1, -1, 0};

static GLuint blank_texture = 0;

static enum RenderTypes render_stage = RT_MAIN;
static struct MainBatch main_batch = {0};
static struct WorldBatch world_batch = {0};

static struct Shader* default_shaders[RT_SIZE] = {NULL};
static struct Shader* current_shader = NULL;
static struct Font* default_font = NULL;

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
    glClearColor(0, 0, 0, 1);
    glm_mat4_identity(model_matrix);
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(projection_matrix);
    glm_ortho(0, 640, 480, 0, -1000, 1000, projection_matrix);
    glm_mat4_mul(view_matrix, model_matrix, mvp_matrix);
    glm_mat4_mul(projection_matrix, mvp_matrix, mvp_matrix);

    INFO("Opened");
}

void video_init_render() {
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
    main_batch.blend_src[0] = GL_SRC_ALPHA;
    main_batch.blend_src[1] = GL_SRC_ALPHA;
    main_batch.blend_dest[0] = GL_ONE_MINUS_SRC_ALPHA;
    main_batch.blend_dest[1] = GL_ONE;
    main_batch.filter = false;
    glEnable(GL_BLEND);

    // Shaders
    if (default_shaders[RT_MAIN] == NULL && (default_shaders[RT_MAIN] = fetch_shader("main")) == NULL)
        FATAL("Main shader \"main\" not found");

    // Fonts
    if ((default_font = fetch_font("main")) == NULL)
        FATAL("Main font \"main\" not found");

    INFO("Opened for rendering");
}

static int dummy = 0;
static struct Texture* dumtex = NULL;
void video_update() {
    glClear(GL_COLOR_BUFFER_BIT);

    // World
    glViewport(0, 0, display.width, display.height);
    set_render_stage(RT_WORLD);
    submit_world_batch();
    // ...

    // Main
    const float scale = SDL_min(
        (float)display.width / (float)(DEFAULT_DISPLAY_WIDTH), (float)display.height / (float)(DEFAULT_DISPLAY_HEIGHT)
    );
    const float width = (float)(DEFAULT_DISPLAY_WIDTH)*scale;
    const float height = (float)(DEFAULT_DISPLAY_HEIGHT)*scale;
    glViewport(((float)display.width - width) / 2, ((float)display.height - height) / 2, width, height);

    render_stage = RT_MAIN;
    set_shader(NULL);
    if (!dummy) {
        dumtex = fetch_texture("dumdum");
        dummy = 1;
    }

    /*set_main_texture(dumtex);
    main_vertex(0, 480, 0, 255, 0, 0, 255, 0, 1);
    main_vertex(640, 480, 0, 0, 255, 0, 255, 1, 1);
    main_vertex(320, 0, 0, 0, 0, 255, 255, 0.5, 0);*/
    main_sprite(dumtex, 130, 120, -1);
    // main_sprite(dumtex, 320, 240, -2);
    main_string(
        "  ________________\n"
        "< go fuck yourself >\n"
        "  ----------------\n"
        "         \\   ^__^\n"
        "          \\  (oo)\\_______\n"
        "             (__)\\       )\\/\\\\\n"
        "                 ||----w |\n"
        "                 ||     ||",
        NULL, 16, 0, 0, -8
    );
    main_string_wrap(
        "I am a string. Look at me I am wrapping. There I wrapped again. It just never stops. I actually can't stop.",
        NULL, 16, 100, 320, 0, 0
    );
    submit_main_batch();

    SDL_GL_SwapWindow(window);
}

void video_teardown() {
    glDeleteTextures(1, &blank_texture);

    glDeleteVertexArrays(1, &main_batch.vao);
    glDeleteBuffers(1, &main_batch.vbo);
    lame_free(&main_batch.vertices);

    SDL_GL_DestroyContext(gpu);
    SDL_DestroyWindow(window);

    INFO("Closed");
}

// Display
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

    /* Exclusive fullscreen commented out as WinAPI does some weird stuff that
       CRT considers as "memory leaks".
       220 bytes are "leaked" every time you tab out and back in. */
    /*
    if (display.fullscreen == FSM_EXCLUSIVE_FULLSCREEN) {
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
    }
    */
    SDL_SetWindowFullscreen(window, display.fullscreen != FSM_WINDOWED);

    // Vsync
    SDL_GL_SetSwapInterval(display.vsync = vsync);

    // Center and wait
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RestoreWindow(window);
    SDL_SyncWindow(window);

    SDL_GetWindowSize(window, &display.width, &display.height);
    INFO("Display set to %dx%d, mode %d, vsync %d", display.width, display.height, display.fullscreen, display.vsync);
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

extern void set_uint_uniform(const char* name, GLuint value) {
    glUniform1ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

extern void set_uvec2_uniform(const char* name, GLuint x, GLuint y) {
    glUniform2ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

extern void set_uvec3_uniform(const char* name, GLuint x, GLuint y, GLuint z) {
    glUniform3ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

extern void set_uvec4_uniform(const char* name, GLuint x, GLuint y, GLuint z, GLuint w) {
    glUniform4ui((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

extern void set_int_uniform(const char* name, GLint value) {
    glUniform1i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

extern void set_ivec2_uniform(const char* name, GLint x, GLint y) {
    glUniform2i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

extern void set_ivec3_uniform(const char* name, GLint x, GLint y, GLint z) {
    glUniform3i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

extern void set_ivec4_uniform(const char* name, GLint x, GLint y, GLint z, GLint w) {
    glUniform4i((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

extern void set_float_uniform(const char* name, GLfloat value) {
    glUniform1f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), value);
}

extern void set_vec2_uniform(const char* name, GLfloat x, GLfloat y) {
    glUniform2f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y);
}

extern void set_vec3_uniform(const char* name, GLfloat x, GLfloat y, GLfloat z) {
    glUniform3f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z);
}

extern void set_vec4_uniform(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    glUniform4f((GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), x, y, z, w);
}

extern void set_mat2_uniform(const char* name, mat2* matrix) {
    glUniformMatrix2fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

extern void set_mat3_uniform(const char* name, mat3* matrix) {
    glUniformMatrix3fv(
        (GLint)SDL_GetNumberProperty(current_shader->uniforms, name, -1), 1, GL_FALSE, (const GLfloat*)matrix
    );
}

extern void set_mat4_uniform(const char* name, mat4* matrix) {
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

void set_main_texture(struct Texture* texture) {
    GLuint target = texture == NULL ? blank_texture : texture->texture;

    if (main_batch.texture != target) {
        submit_main_batch();
        main_batch.texture = target;
    }
}

void main_vertex(GLfloat x, GLfloat y, GLfloat z, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat u, GLfloat v) {
    if (main_batch.vertex_count >= main_batch.vertex_capacity) {
        // submit_main_batch();
        main_batch.vertex_capacity *= 2;
        INFO("Reallocated main batch VBO to %u vertices", main_batch.vertex_capacity);
        lame_realloc(&main_batch.vertices, main_batch.vertex_capacity * sizeof(struct MainVertex));
        glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct MainVertex) * main_batch.vertex_capacity, NULL, GL_DYNAMIC_DRAW);
    }

    main_batch.vertices[main_batch.vertex_count++] = (struct MainVertex){
        x, y, z, main_batch.color[0] * r, main_batch.color[1] * g, main_batch.color[2] * b, main_batch.color[3] * a,
        u, v
    };
}

void main_sprite(struct Texture* texture, GLfloat x, GLfloat y, GLfloat z) {
    if (texture == NULL)
        return;
    set_main_texture(texture);

    GLfloat x1 = x;
    GLfloat y1 = y;
    GLfloat x2 = x1 + texture->size[0];
    GLfloat y2 = y1 + texture->size[1];
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
    main_vertex(x2, y2, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[3]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x2, y1, z, 255, 255, 255, 255, texture->uvs[2], texture->uvs[1]);
    main_vertex(x1, y1, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[1]);
    main_vertex(x1, y2, z, 255, 255, 255, 255, texture->uvs[0], texture->uvs[3]);
}

void main_string(const char* str, struct Font* font, GLfloat size, GLfloat x, GLfloat y, GLfloat z) {
    if (font == NULL)
        font = default_font;
    set_main_texture(hid_to_texture(font->texture));

    GLfloat scale = size / font->size;
    GLfloat cx = x;
    GLfloat cy = y;
    for (size_t i = 0, n = SDL_strlen(str); i < n; i++) {
        unsigned char gid = str[i];
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

        struct Glyph* glyph = &font->glyphs[gid];
        GLfloat x1 = cx + (glyph->offset[0] * scale);
        GLfloat y1 = cy + (glyph->offset[1] * scale);
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
    GLfloat cx = x;
    GLfloat cy = y;

    // https://github.com/raysan5/raylib/blob/master/examples/text/text_rectangle_bounds.c
    size_t n = SDL_strlen(str);
    bool measure = true;
    int start_pos = -1, end_pos = -1;

    for (int i = 0; i < n; i++) {
        unsigned char gid = str[i];
        GLfloat gwidth = (gid >= font->num_glyphs || gid == '\n') ? 0 : (font->glyphs[gid].advance * scale);

        if (measure) {
            if (SDL_isspace(gid))
                end_pos = i;

            if (((cx + gwidth) - x) > width) {
                end_pos = (end_pos < 1) ? i : end_pos;
                if (i == end_pos)
                    end_pos -= 1;
                if ((start_pos + 1) == end_pos)
                    end_pos = i - 1;
                measure = !measure;
            } else if ((i + 1) == n) {
                end_pos = i;
                measure = !measure;
            } else if (gid == '\n') {
                measure = !measure;
            }

            if (!measure) {
                cx = x;
                i = start_pos;
                gwidth = 0;
            }
        } else {
            if (gid != '\n' && gid < font->num_glyphs) {
                struct Glyph* glyph = &font->glyphs[gid];
                GLfloat x1 = cx + (glyph->offset[0] * scale);
                GLfloat y1 = cy + (glyph->offset[1] * scale);
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
                cy += size;
                cx = x;
                start_pos = end_pos;
                end_pos = 0;
                measure = !measure;
            }
        }

        if (cx > x || !SDL_isspace(gid))
            cx += gwidth;
    }
}

// World
void submit_world_batch() {}
