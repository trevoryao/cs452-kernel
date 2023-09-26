#ifndef __KASSERT_H__
#define __KASSERT_H__

// lib
#include "rpi.h"

// asserts and panics (kernel)

#define kassert(expr) \
    ((__builtin_expect(!(expr), 0)) ? __builtin_trap() : (void)0)

#define kpanic(...) \
    uart_printf(CONSOLE, __VA_ARGS__); \
    __builtin_trap()

#ifdef LOG
#define KLOG(fmt, ...) uart_printf(CONSOLE, "[KLOG] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define KLOG(...)
#endif

#endif
