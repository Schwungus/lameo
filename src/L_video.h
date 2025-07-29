#pragma once

// #include order matters!
// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
// clang-format on

#include "L_asset.h"
#include "L_math.h" // IWYU pragma: keep

#define CHECK_GL_EXTENSION(ext)                                                                                        \
    if (!ext)                                                                                                          \
        FATAL(                                                                                                         \
            "Missing OpenGL extension: " #ext "\nAt least OpenGL 3.3 with framebuffer and shader support is required." \
        );

#define DEFAULT_DISPLAY_WIDTH 640
#define DEFAULT_DISPLAY_HEIGHT 480

#define SURFACE_COLOR_TEXTURE 0
#define SURFACE_DEPTH_TEXTURE 1

#define MAX_BONES 128

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
    GLfloat bone_index[4];
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

struct Surface {
    bool active;
    struct Surface* stack;

    bool enabled[2];
    GLuint fbo, texture[2];
    uint16_t size[2];
};

struct ModelInstance {
    struct Model* model;
    int userdata;

    vec3 pos, angle, scale;
    vec3 draw_pos[2], draw_angle[2], draw_scale[2];

    bool* hidden;
    struct Material** override_materials;
    GLuint* override_textures;

    struct Animation* animation;
    bool loop;
    float frame, frame_speed;

    versor *translations, *rotations;
    DualQuaternion *transforms, *sample;

    DualQuaternion* draw_sample[2];
};

void video_init(bool);
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
void set_main_texture_direct(GLuint);
void set_main_filter(bool);

void main_vertex(GLfloat, GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat);
void main_rectangle(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte);
void main_surface(struct Surface*, GLfloat, GLfloat, GLfloat);
void main_sprite(struct Texture*, GLfloat, GLfloat, GLfloat);
void main_material_sprite(struct Material*, GLfloat, GLfloat, GLfloat);
void main_string(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat);
void main_string_wrap(const char*, struct Font*, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);

// World
void submit_world_batch();
void set_world_color(GLfloat, GLfloat, GLfloat);
void set_world_alpha(GLfloat);
void set_world_stencil_color(GLfloat, GLfloat, GLfloat);
void set_world_stencil_alpha(GLfloat);
void set_world_alpha_test(GLfloat);
void set_world_bright(GLfloat);
void set_world_texture(struct Texture*);
void set_world_texture_direct(GLuint);
void set_world_filter(bool);

void world_vertex(
    GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat
);

struct ActorCamera* get_active_camera();
void set_active_camera(struct ActorCamera*);
struct Surface* render_camera(struct ActorCamera*, uint16_t, uint16_t, bool, struct Shader*, int);

// Fonts
GLfloat string_width(const char*, struct Font*, GLfloat);
GLfloat string_height(const char*, GLfloat);

// Surfaces
struct Surface* create_surface(bool, uint16_t, uint16_t, bool, bool);
void validate_surface(struct Surface*);
void dispose_surface(struct Surface*);
void destroy_surface(struct Surface*);
void set_surface(struct Surface*);
void pop_surface();
void resize_surface(struct Surface*, uint16_t, uint16_t);

void clear_color(GLfloat, GLfloat, GLfloat, GLfloat);
void clear_depth(GLfloat);
void clear_stencil(GLint);

// Model Instances
struct ModelInstance* create_model_instance(struct Model*);
void destroy_model_instance(struct ModelInstance*);
void set_model_instance_animation(struct ModelInstance*, struct Animation*, float, bool);
void translate_model_instance_node(struct ModelInstance*, size_t, versor);
void rotate_model_instance_node(struct ModelInstance*, size_t, versor);
void tick_model_instance(struct ModelInstance*);
void submit_model_instance(struct ModelInstance*);
void draw_model_instance(struct ModelInstance*);
