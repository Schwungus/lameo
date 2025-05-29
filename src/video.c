#include "video.h"
#include "asset.h"
#include "log.h"
#include "mem.h"

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

static GLuint blank_texture = 0;

static enum RenderTypes render_stage = RT_MAIN;
static struct MainBatch main_batch = {0};
static struct WorldBatch world_batch = {0};

static struct Shader* default_shaders[RT_SIZE] = {NULL};
static struct Shader* current_shader = NULL;

void video_init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("lameo", 640, 480, SDL_WINDOW_OPENGL);
    if (window == NULL)
        FATAL("Window fail: %s", SDL_GetError());

    gpu = SDL_GL_CreateContext(window);
    if (gpu == NULL)
        FATAL("GPU fail: %s", SDL_GetError());

    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0)
        FATAL("Failed to load OpenGL functions");
    INFO("GLAD version: %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    SDL_GL_SetSwapInterval(0);

    INFO("OpenGL vendor: %s", glGetString(GL_VENDOR));
    INFO("OpenGL version: %s", glGetString(GL_VERSION));
    INFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
    INFO("OpenGL shading language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glClearColor(0, 0, 0, 1);

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

    main_batch.vertices = lame_alloc(3 * sizeof(struct MainVertex));
    main_batch.vertex_count = 0;
    main_batch.vertex_capacity = 3;

    glGenBuffers(1, &main_batch.vbo);
    glBindVertexArray(main_batch.vao);
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
    main_batch.stencil[0] = main_batch.stencil[1] = main_batch.stencil[2] = 255;
    main_batch.stencil[3] = 0;
    main_batch.texture = blank_texture;
    main_batch.blend_src[0] = GL_SRC_COLOR;
    main_batch.blend_src[1] = GL_SRC_ALPHA;
    main_batch.blend_dest[0] = GL_DST_COLOR;
    main_batch.blend_dest[1] = GL_DST_ALPHA;

    // Shaders
    if (default_shaders[RT_MAIN] == NULL && (default_shaders[RT_MAIN] = fetch_shader("main")) == NULL)
        FATAL("Main shader \"main\" not found");

    INFO("Opened for rendering");
}

static int dummy = 0;
static TextureID dumtex = 0;
void video_update() {
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, 640, 480);

    // World
    set_render_stage(RT_WORLD);
    submit_world_batch();
    // ...

    // Main
    render_stage = RT_MAIN;
    set_shader(NULL);
    if (!dummy) {
        dumtex = fetch_texture_hid("dumdum");
        dummy = 1;
    }
    set_main_texture(hid_to_texture(dumtex));
    main_vertex(-0.5, -0.5, 0, 255, 0, 0, 255, 0, 1);
    main_vertex(0.5, -0.5, 0, 0, 255, 0, 255, 1, 1);
    main_vertex(0, 0.5, 0, 0, 0, 255, 255, 0.5, 0);
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, main_batch.texture);
    set_int_uniform("u_texture", 0);
    glBindBuffer(GL_ARRAY_BUFFER, main_batch.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct MainVertex) * main_batch.vertex_count, main_batch.vertices);
    glBindVertexArray(main_batch.vao);
    glBlendFuncSeparate(
        main_batch.blend_src[0], main_batch.blend_dest[0], main_batch.blend_src[1], main_batch.blend_dest[1]
    );
    glDrawArrays(GL_TRIANGLES, 0, main_batch.vertex_count);
    main_batch.vertex_count = 0;
}

void set_main_texture(struct Texture* texture) {
    GLint target = texture == NULL ? blank_texture : texture->texture;

    if (main_batch.texture != target) {
        submit_main_batch();
        main_batch.texture = target;
    }
}

void main_vertex(GLfloat x, GLfloat y, GLfloat z, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat u, GLfloat v) {
    if (main_batch.vertex_count >= main_batch.vertex_capacity) {
        main_batch.vertex_capacity *= 2;
        INFO("Reallocated main batch VBO to %u vertices", main_batch.vertex_capacity);
        lame_realloc(&main_batch.vertices, main_batch.vertex_capacity * sizeof(struct MainVertex));
    }

    main_batch.vertices[main_batch.vertex_count++] = (struct MainVertex){
        x, y, z, main_batch.color[0] * r, main_batch.color[1] * g, main_batch.color[2] * b, main_batch.color[3] * a,
        u, v
    };
}

// World
void submit_world_batch() {}
