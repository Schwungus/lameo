#pragma once

// #include order matters!
// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
// clang-format on

enum RenderTypes {
    RT_MAIN,
    RT_WORLD,
    RT_SIZE,
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

void video_init();
void video_init_render();
void video_update();
void video_teardown();
