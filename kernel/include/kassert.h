#ifndef __KASSERT_H__
#define __KASSERT_H__

#include <rpi.h>

// asserts and panics (kernel)

#define kassert(expr) \
    ((__builtin_expect(!(expr), 0)) ? __builtin_trap() : (void)0)

#define panic(...) \
    uart_printf(CONSOLE, __VA_ARGS__); \
    __builtin_trap()

#endif
