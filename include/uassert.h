#ifndef __ASSERT_H__
#define __ASSERT_H__

// lib
#include "task.h"
#include "rpi.h"

// asserts and panics (user tasks)

#define uassert(expr) \
    if (__builtin_expect(!(expr), 0)) { \
        uart_printf(CONSOLE, "Assert Failed (%s:%d -- %s) %s\r\n\r\n", __FILE__, __LINE__, __func__, #expr); \
        while (!uart_out_empty(CONSOLE)); \
        __builtin_trap(); \
    }

#define upanic(fmt, ...) \
    uart_printf(CONSOLE, "PANIC: " fmt "\r\n" __VA_OPT__(,) __VA_ARGS__); \
    while (!uart_out_empty(CONSOLE)); \
    __builtin_trap()

#endif
