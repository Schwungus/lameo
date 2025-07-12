#pragma once

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "cglm/cglm.h" // IWYU pragma: keep

#include "L_log.h"

#define PRINT_MAT4(matrix)                                                                                             \
    INFO(                                                                                                              \
        #matrix ": \n%.2f, %.2f, %.2f, %.2f\n%.2f, %.2f, %.2f, %.2f\n%.2f, %.2f, %.2f, %.2f\n%.2f, %.2f, %.2f, %.2f",  \
        matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3], matrix[1][0], matrix[1][1], matrix[1][2],              \
        matrix[1][3], matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3], matrix[3][0], matrix[3][1],              \
        matrix[3][2], matrix[3][3]                                                                                     \
    );

#define deg_to_rad(deg) ((deg) * (SDL_PI_F / 180))
#define rad_to_deg(rad) ((rad) * (180 / SDL_PI_F))

float lerp(float, float, float);
