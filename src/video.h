#pragma once

// #include order matters!
// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
// clang-format on

#include "asset.h"
#include "math.h" // IWYU pragma: keep

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

    GLfloat color[4];
    GLfloat stencil[4];
    GLuint texture;
    GLfloat alpha_test;
    GLenum blend_src[2], blend_dest[2];
    bool filter;
};

struct WorldBatch {
    GLuint vao, vbo;
    size_t vertex_count, vertex_capacity;
    struct WorldVertex* vertices;

    GLfloat color[4];
    GLfloat stencil[4];
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
void set_display(int, int, enum FullscreenModes, bool);
void set_framerate(uint16_t);

// Shaders 'n' uniforms
void set_shader(struct Shader*);

inline void set_uint_uniform(const char*, GLuint);
inline void set_uvec2_uniform(const char*, GLuint, GLuint);
inline void set_uvec3_uniform(const char*, GLuint, GLuint, GLuint);
inline void set_uvec4_uniform(const char*, GLuint, GLuint, GLuint, GLuint);

inline void set_int_uniform(const char*, GLint);
inline void set_ivec2_uniform(const char*, GLint, GLint);
inline void set_ivec3_uniform(const char*, GLint, GLint, GLint);
inline void set_ivec4_uniform(const char*, GLint, GLint, GLint, GLint);

inline void set_float_uniform(const char*, GLfloat);
inline void set_vec2_uniform(const char*, GLfloat, GLfloat);
inline void set_vec3_uniform(const char*, GLfloat, GLfloat, GLfloat);
inline void set_vec4_uniform(const char*, GLfloat, GLfloat, GLfloat, GLfloat);

inline void set_mat2_uniform(const char*, mat2*);
inline void set_mat3_uniform(const char*, mat3*);
inline void set_mat4_uniform(const char*, mat4*);

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
inline void set_main_batch(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, bool);

void main_vertex(GLfloat, GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat);
void main_sprite(struct Texture*, GLfloat, GLfloat, GLfloat);
void main_string(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat);
void main_string_wrap(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);

// World
void submit_world_batch();
