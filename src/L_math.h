#pragma once

#define CGLM_ALL_UNALIGNED
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

#define PRINT_DQ(dq)                                                                                                   \
    INFO(                                                                                                              \
        #dq ": \n%.2f, %.2f, %.2f, %.2f\n%.2f, %.2f, %.2f, %.2f", dq[0], dq[1], dq[2], dq[3], dq[4], dq[5], dq[6],     \
        dq[7]                                                                                                          \
    );

float angle_difference(float, float);
float lerp_angle(float, float, float);

// Dual Quaternions
typedef float DualQuaternion[8];

#define DQ_IDENTITY (DualQuaternion){0, 0, 0, 1, 0, 0, 0, 0}
#define DQ_IDENTITY_INIT {0, 0, 0, 1, 0, 0, 0, 0}

void dq_identity(float[8]);
void dq_copy(const float[8], float[8]);
void dq_mul(const float[8], const float[8], float[8]);
void dq_lerp(const float[8], const float[8], float, float[8]);
void dq_slerp(const float[8], const float[8], float, float[8]);
