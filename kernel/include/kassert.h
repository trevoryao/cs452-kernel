#ifndef __KASSERT_H__
#define __KASSERT_H__

// lib
#include "rpi.h"

// asserts and panics (kernel)

#define kassert(expr) \
    if (__builtin_expect(!(expr), 0)) { \
        uart_printf(CONSOLE, "Kernel Assert Failed (%s:%d -- %s) %s\r\n\r\n", __FILE__, __LINE__, __func__, #expr); \
        while (!uart_out_empty(CONSOLE)); \
        __builtin_trap(); \
    }

#define kpanic(fmt, ...) \
    uart_printf(CONSOLE, "KERNEL PANIC: " fmt "\r\n" __VA_OPT__(,) __VA_ARGS__); \
    while (!uart_out_empty(CONSOLE)); \
    __builtin_trap()

#ifdef LOG
#define KLOG(fmt, ...) uart_printf(CONSOLE, "[KLOG] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define KLOG(...)
#endif

#endif
