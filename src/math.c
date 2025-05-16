#include "math.h"

extern float lerp(float a, float b, float c) {
    return a + ((b - a) * c);
}
