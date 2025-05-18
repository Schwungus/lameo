#include "video.h"
#include "asset.h"
#include "log.h"
#include "mem.h"

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

static GLuint blank_texture = 0;
static GLuint rformats[RT_SIZE] = {0};

static struct Shader* rshaders[RT_SIZE] = {NULL};
static struct Shader* current_shader = NULL;

void video_init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

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

// clang-format off
static struct MainVertex vertices[] = {
  // position------   color----------   uv--
    {-0.5, -0.5, 0,   255, 0, 0, 255,   0, 0},
    {0.5,  -0.5, 0,   0, 255, 0, 255,   0, 0},
    {0,     0.5, 0,   0, 0, 255, 255,   0, 0},
};
// clang-format on
static GLuint vbo = 0;

void video_init_render() {
    // Blank texture
    glGenTextures(1, &blank_texture);
    glBindTexture(GL_TEXTURE_2D, blank_texture);
    static const uint8_t pixel[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // Vertex Formats
    glCreateVertexArrays(RT_SIZE, rformats);

    // Main format
    // in vec3 i_position; (X, Y, Z)
    glVertexArrayAttribFormat(rformats[RT_MAIN], SHAT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(rformats[RT_MAIN], SHAT_POSITION);
    // in vec4 i_color; (R, G, B, A)
    glVertexArrayAttribFormat(rformats[RT_MAIN], SHAT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(rformats[RT_MAIN], SHAT_COLOR);
    // in vec2 i_uv; (U, V)
    glVertexArrayAttribFormat(rformats[RT_MAIN], SHAT_UV, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2);
    glEnableVertexArrayAttrib(rformats[RT_MAIN], SHAT_UV);

    // World format
    // in vec3 i_position; (X, Y, Z)
    glVertexArrayAttribFormat(rformats[RT_WORLD], 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 0);
    // in vec3 i_normal; (X, Y, Z)
    glVertexArrayAttribFormat(rformats[RT_WORLD], 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 1);
    // in vec4 i_color; (R, G, B, A)
    glVertexArrayAttribFormat(rformats[RT_WORLD], 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 2);
    // in vec4 i_uv; (U, V, U, V)
    glVertexArrayAttribFormat(rformats[RT_WORLD], 3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 3);
    // in ivec4 i_bone_index; (I, J, K, L)
    glVertexArrayAttribIFormat(rformats[RT_WORLD], 4, 4, GL_UNSIGNED_BYTE, sizeof(GLubyte) * 4);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 4);
    // in vec4 i_bone_weight; (X, Y, Z, W)
    glVertexArrayAttribFormat(rformats[RT_WORLD], 5, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GLfloat) * 4);
    glEnableVertexArrayAttrib(rformats[RT_WORLD], 5);

    // Dummy VBO
    glGenBuffers(1, &vbo);
    glBindVertexArray(rformats[RT_MAIN]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(SHAT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct MainVertex), (void*)0);
    glVertexAttribPointer(
        SHAT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct MainVertex), (void*)(3 * sizeof(GLfloat))
    );
    glVertexAttribPointer(
        SHAT_UV, 2, GL_FLOAT, GL_FALSE, sizeof(struct MainVertex),
        (void*)((3 * sizeof(GLfloat)) + (4 * sizeof(GLubyte)))
    );
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Shaders
    if (rshaders[RT_MAIN] == NULL && (rshaders[RT_MAIN] = fetch_shader("main")) == NULL)
        FATAL("Main shader \"main\" not found");

    INFO("Opened for rendering");
}

void video_update() {
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, 640, 480);
    glUseProgram(rshaders[RT_MAIN]->program);
    glBindTexture(GL_TEXTURE_2D, blank_texture);
    glBindVertexArray(rformats[RT_MAIN]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
}

void video_teardown() {
    glDeleteVertexArrays(RT_SIZE, rformats);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &blank_texture);

    SDL_GL_DestroyContext(gpu);
    SDL_DestroyWindow(window);

    INFO("Closed");
}
