#pragma once

// #include order matters!
// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
// clang-format on

#include "L_asset.h"
#include "L_math.h" // IWYU pragma: keep

#define DEFAULT_DISPLAY_WIDTH 640
#define DEFAULT_DISPLAY_HEIGHT 480

enum FullscreenModes {
    FSM_WINDOWED,
    FSM_FULLSCREEN,
    FSM_EXCLUSIVE_FULLSCREEN,
};

enum RenderTypes {
    RT_MAIN,
    RT_WORLD,
    RT_SIZE,
};

enum VertexAttributes {
    VATT_POSITION,
    VATT_NORMAL,
    VATT_COLOR,
    VATT_UV,
    VATT_BONE_INDEX,
    VATT_BONE_WEIGHT,
    VATT_SIZE,
};

struct Display {
    int width, height;
    enum FullscreenModes fullscreen;
    bool vsync;
};

struct MainVertex {
    GLfloat position[3];
    GLubyte color[4];
    GLfloat uv[2];
};

struct WorldVertex {
    GLfloat position[3];
    GLfloat normal[3];
    GLubyte color[4];
    GLfloat uv[4];
    GLubyte bone_index[4];
    GLfloat bone_weight[4];
};

struct MainBatch {
    GLuint vao, vbo;
    size_t vertex_count, vertex_capacity;
    struct MainVertex* vertices;

    GLfloat color[4], stencil[4];
    GLuint texture;
    GLfloat alpha_test;
    GLenum blend_src[2], blend_dest[2];
    bool filter;
};

struct WorldBatch {
    GLuint vao, vbo;
    size_t vertex_count, vertex_capacity;
    struct WorldVertex* vertices;

    GLfloat color[4], stencil[4];
    GLuint texture;
    GLfloat alpha_test, bright;
    GLenum blend_src[2], blend_dest[2];
    bool filter;
};

void video_init();
void video_init_render();
void video_update();
void video_teardown();

// Display
const struct Display* get_display();
void set_display(int, int, enum FullscreenModes, bool);
void set_framerate(uint16_t);

uint64_t get_draw_time();

// Shaders 'n' uniforms
void set_shader(struct Shader*);

void set_uint_uniform(const char*, GLuint);
void set_uvec2_uniform(const char*, GLuint, GLuint);
void set_uvec3_uniform(const char*, GLuint, GLuint, GLuint);
void set_uvec4_uniform(const char*, GLuint, GLuint, GLuint, GLuint);

void set_int_uniform(const char*, GLint);
void set_ivec2_uniform(const char*, GLint, GLint);
void set_ivec3_uniform(const char*, GLint, GLint, GLint);
void set_ivec4_uniform(const char*, GLint, GLint, GLint, GLint);

void set_float_uniform(const char*, GLfloat);
void set_vec2_uniform(const char*, GLfloat, GLfloat);
void set_vec3_uniform(const char*, GLfloat, GLfloat, GLfloat);
void set_vec4_uniform(const char*, GLfloat, GLfloat, GLfloat, GLfloat);

void set_mat2_uniform(const char*, mat2*);
void set_mat3_uniform(const char*, mat3*);
void set_mat4_uniform(const char*, mat4*);

// Render stages
void set_render_stage(enum RenderTypes);
void submit_batch();

// Main
void submit_main_batch();
void set_main_color(GLfloat, GLfloat, GLfloat);
void set_main_alpha(GLfloat);
void set_main_stencil_color(GLfloat, GLfloat, GLfloat);
void set_main_stencil_alpha(GLfloat);
void set_main_alpha_test(GLfloat);
void set_main_texture(struct Texture*);
void set_main_filter(bool);
void set_main_batch(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, bool);

void main_vertex(GLfloat, GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat);
void main_sprite(struct Texture*, GLfloat, GLfloat, GLfloat);
void main_material_sprite(struct Material*, GLfloat, GLfloat, GLfloat);
void main_string(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat);
void main_string_wrap(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);

// World
void submit_world_batch();

// Fonts
GLfloat string_width(const char*, struct Font*, GLfloat);
GLfloat string_height(const char*, GLfloat);
