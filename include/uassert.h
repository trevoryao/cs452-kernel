#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <task.h>
#include <rpi.h>

// asserts and panics (user tasks)

#define uassert(expr) \
    ((__builtin_expect(!(expr), 0)) ? Exit() : (void)0)

#define upanic(...) \
    uart_printf(CONSOLE, __VA_ARGS__); \
    Exit()

#endif
