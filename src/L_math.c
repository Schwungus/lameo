#include "L_math.h"

float angle_difference(float a, float b) {
    return SDL_fmodf((SDL_fmodf(a - b, 360) + 540), 360) - 180;
}

float lerp_angle(float a, float b, float x) {
    return a + (x * angle_difference(b, a));
}

void dq_identity(float dest[8]) {
    dest[0] = 0;
    dest[1] = 0;
    dest[2] = 0;
    dest[3] = 1;
    dest[4] = 0;
    dest[5] = 0;
    dest[6] = 0;
    dest[7] = 0;
}

void dq_copy(const float dq[8], float dest[8]) {
    dest[0] = dq[0];
    dest[1] = dq[1];
    dest[2] = dq[2];
    dest[3] = dq[3];
    dest[4] = dq[4];
    dest[5] = dq[5];
    dest[6] = dq[6];
    dest[7] = dq[7];
}

void dq_mul(const float a[8], const float b[8], float dest[8]) {
    float a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5], a6 = a[6], a7 = a[7];
    float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5], b6 = b[6], b7 = b[7];
    dest[0] = (b3 * a0 + b0 * a3 + b1 * a2 - b2 * a1);
    dest[1] = (b3 * a1 + b1 * a3 + b2 * a0 - b0 * a2);
    dest[2] = (b3 * a2 + b2 * a3 + b0 * a1 - b1 * a0);
    dest[3] = (b3 * a3 - b0 * a0 - b1 * a1 - b2 * a2);
    dest[4] = (b7 * a0 + b4 * a3 + b5 * a2 - b6 * a1) + (b3 * a4 + b0 * a7 + b1 * a6 - b2 * a5);
    dest[5] = (b7 * a1 + b5 * a3 + b6 * a0 - b4 * a2) + (b3 * a5 + b1 * a7 + b2 * a4 - b0 * a6);
    dest[6] = (b7 * a2 + b6 * a3 + b4 * a1 - b5 * a0) + (b3 * a6 + b2 * a7 + b0 * a5 - b1 * a4);
    dest[7] = (b7 * a3 - b4 * a0 - b5 * a1 - b6 * a2) + (b3 * a7 - b0 * a4 - b1 * a5 - b2 * a6);
}

void dq_lerp(const float a[8], const float b[8], float amount, float dest[8]) {
    float a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5], a6 = a[6], a7 = a[7];
    float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5], b6 = b[6], b7 = b[7];
    dest[0] = glm_lerp(a0, b0, amount);
    dest[1] = glm_lerp(a1, b1, amount);
    dest[2] = glm_lerp(a2, b2, amount);
    dest[3] = glm_lerp(a3, b3, amount);
    dest[4] = glm_lerp(a4, b4, amount);
    dest[5] = glm_lerp(a5, b5, amount);
    dest[6] = glm_lerp(a6, b6, amount);
    dest[7] = glm_lerp(a7, b7, amount);
}

void dq_slerp(const float a[8], const float b[8], float amount, float dest[8]) {
    // First dual quaternion
    float a0 = a[0];
    float a1 = a[1];
    float a2 = a[2];
    float a3 = a[3];
    // (* 2 since we use this only in the translation reconstruction)
    float a4 = a[4] * 2;
    float a5 = a[5] * 2;
    float a6 = a[6] * 2;
    float a7 = a[7] * 2;

    // Second dual quaternion
    float b0 = b[0];
    float b1 = b[1];
    float b2 = b[2];
    float b3 = b[3];
    // (* 2 since we use this only in the translation reconstruction)
    float b4 = b[4] * 2;
    float b5 = b[5] * 2;
    float b6 = b[6] * 2;
    float b7 = b[7] * 2;

    // Lerp between reconstructed translations
    float p0 = glm_lerp(
        a7 * (-a0) + a4 * a3 + a5 * (-a2) - a6 * (-a1), b7 * (-b0) + b4 * b3 + b5 * (-b2) - b6 * (-b1), amount
    );
    float p1 = glm_lerp(
        a7 * (-a1) + a5 * a3 + a6 * (-a0) - a4 * (-a2), b7 * (-b1) + b5 * b3 + b6 * (-b0) - b4 * (-b2), amount
    );
    float p2 = glm_lerp(
        a7 * (-a2) + a6 * a3 + a4 * (-a1) - a5 * (-a0), b7 * (-b2) + b6 * b3 + b4 * (-b1) - b5 * (-b0), amount
    );

    // Slerp rotations
    float n = 1 / SDL_sqrtf(a0 * a0 + a1 * a1 + a2 * a2 + a3 * a3);
    a0 *= n;
    a1 *= n;
    a2 *= n;
    a3 *= n;
    n = SDL_sqrtf(b0 * b0 + b1 * b1 + b2 * b2 + b3 * b3);
    b0 *= n;
    b1 *= n;
    b2 *= n;
    b3 *= n;

    float d = a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3;
    if (d < 0) {
        d = -d;
        b0 *= -1;
        b1 *= -1;
        b2 *= -1;
        b3 *= -1;
    }
    if (d > 0.9995) {
        a0 = glm_lerp(a0, b0, amount);
        a1 = glm_lerp(a1, b1, amount);
        a2 = glm_lerp(a2, b2, amount);
        a3 = glm_lerp(a3, b3, amount);
    } else {
        float t0 = SDL_acosf(d);
        float t = t0 * amount;
        float s2 = SDL_sinf(t) / sinf(t0);
        float s1 = SDL_cosf(t) - (d * s2);
        a0 = (a0 * s1) + (b0 * s2);
        a1 = (a1 * s1) + (b1 * s2);
        a2 = (a2 * s1) + (b2 * s2);
        a3 = (a3 * s1) + (b3 * s2);
    }

    // Recreate dual quaternion from translation and rotation
    dest[0] = a0;
    dest[1] = a1;
    dest[2] = a2;
    dest[3] = a3;
    dest[4] = (p0 * a3 + p1 * a2 - p2 * a1) * 0.5f;
    dest[5] = (p1 * a3 + p2 * a0 - p0 * a2) * 0.5f;
    dest[6] = (p2 * a3 + p0 * a1 - p1 * a0) * 0.5f;
    dest[7] = (-p0 * a0 - p1 * a1 - p2 * a2) * 0.5f;
}
