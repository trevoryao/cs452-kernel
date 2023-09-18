#ifndef _util_h_
#define _util_h_ 1

#include <stddef.h>
#include <task.h>

// conversions
int a2d(char ch);
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );

// memory
void *memset(void *s, int c, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);

// asserts and panics
#define kassert(expr) \
    ((__builtin_expect(!(expr), 0)) ? __builtin_trap() : (void)0)
#define assert(expr) \
    ((__builtin_expect(!(expr), 0)) ? Exit() : (void)0)

#endif /* util.h */
