#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_video.h>

#include "log.h"
#include "mem.h"
#include "video.h"

static SDL_Window* window = NULL;
static SDL_GLContext gpu = NULL;

void video_init() {
    window = SDL_CreateWindow("lameo", 640, 480, SDL_WINDOW_OPENGL);
    if (window == NULL)
        FATAL("Window fail: %s", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    gpu = SDL_GL_CreateContext(window);
    if (gpu == NULL)
        FATAL("GPU fail: %s", SDL_GetError());
    SDL_GL_SetSwapInterval(0);

    INFO("OpenGL version: %s", glGetString(GL_VERSION));
    INFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
    INFO("OpenGL shading language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    INFO("Opened");
}

static float white = 0;
void video_update() {
    white += 0.01;
    if (white > 1)
        white = 0;
    glClearColor(white, white, white, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
}

void video_teardown() {
    CLOSE_POINTER(gpu, SDL_GL_DestroyContext);
    CLOSE_POINTER(window, SDL_DestroyWindow);

    INFO("Closed");
}
