#include "L_math.h"

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
