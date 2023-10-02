#ifndef _util_h_
#define _util_h_ 1

#include <stddef.h>

// conversions
int a2d(char ch);
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );

// memory
void *memset(void *s, int c, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);

// strings
size_t strlen(const char *str);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);


#endif /* util.h */
