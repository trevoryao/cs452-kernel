#ifndef __ASSERT_H__
#define __ASSERT_H__

// lib
#include "task.h"
#include "term-control.h"
#include "rpi.h"

// asserts and panics (user tasks)

#define uassert(expr) \
    if (__builtin_expect(!(expr), 0)) { \
        uart_printf(CONSOLE, "Assert Failed (%s:%d -- %s) %s\r\n\r\n", __FILE__, __LINE__, __func__, #expr); \
        while (!uart_is_tx_clear(CONSOLE)); \
        __builtin_trap(); \
    }

#define upanic(fmt, ...) \
    uart_printf(CONSOLE, "PANIC: " fmt "\r\n" __VA_OPT__(,) __VA_ARGS__); \
    while (!uart_is_tx_clear(CONSOLE)); \
    __builtin_trap()

#if defined(LOG) || defined(USERLOG)
#define ULOG(fmt, ...) uart_printf(CONSOLE, COL_YEL "[ULOG] " fmt COL_RST __VA_OPT__(,) __VA_ARGS__)
#else
#define ULOG(...)
#endif

#endif
