#include "util.h"

// ascii digit to integer
int a2d( char ch ) {
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

// unsigned int to ascii string
void ui2a( unsigned int num, unsigned int base, char *bf ) {
	int n = 0;
	int dgt;
	unsigned int d = 1;

	while( (num / d) >= base ) d *= base;
	while( d != 0 ) {
		dgt = num / d;
		num %= d;
		d /= base;
		if( n || dgt > 0 || d == 0 ) {
			*bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
			++n;
		}
	}
	*bf = 0;
}

// signed int to ascii string
void i2a( int num, char *bf ) {
	if( num < 0 ) {
		num = -num;
		*bf++ = '-';
	}
	ui2a( num, 10, bf );
}

// define our own memset to avoid SIMD instructions emitted from the compiler
void *memset(void *s, int c, size_t n) {
  for (char* it = (char*)s; n > 0; --n) *it++ = c;
  return s;
}

// define our own memcpy to avoid SIMD instructions emitted from the compiler
void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
	if (!dest || !src) return dest;

	char *sit = (char *)src;
    char* cdest = (char*)dest;
    for (size_t i = 0; i < n; ++i) *(cdest++) = *(sit++);
    return dest;
}

size_t strlen(const char *str) {
    size_t i = 0;
	while (str[i++] != '\0');
	return i;
}

char *strncpy(char *dest, const char *src, size_t n) {
	if (!dest) return NULL;

	char *it = dest;
	while (*src && (n-- > 0)) { // at most n, or until null-terminated
		*it = *src;
		++it;
		++src;
	}

	*it = '\0'; // append null terminator
	return dest;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n && *s1 && (*s1 == *s2)) {
		++s1;
		++s2;
		--n;
	}

	if (n == 0) return 0;
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}
