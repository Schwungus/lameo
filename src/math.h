#pragma once

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "cglm/cglm.h" // IWYU pragma: keep

#include "log.h"

#define PRINT_MAT4(matrix)                                                                                             \
    INFO(                                                                                                              \
        "\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f", matrix[0][0], matrix[0][1], matrix[0][2],              \
        matrix[0][3], matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3], matrix[2][0], matrix[2][1],              \
        matrix[2][2], matrix[2][3], matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]                             \
    );

float lerp(float, float, float);
