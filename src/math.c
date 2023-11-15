#include "math.h"

uint64_t sqrt(uint64_t n) {
    // newton's method
    // continue until we have no extra refinement
    // NOTE: use right shift by 1 for division by 2

    // initial guess (see https://math.stackexchange.com/a/787041)
    uint64_t x = 1 + ((n - 1) >> 1);

    for (;;) {
        uint64_t x_next = (x + (n / x)) >> 1;
        if (x == x_next) break; // no more refinements
        x = x_next;
    }

    return x;
}
