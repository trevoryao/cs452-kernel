#ifndef __CONSOLE_SERVER_H__
#define __CONSOLE_SERVER_H__

#include <stdint.h>
#include "msg-type.h"

#define ARROW_UP (char)128
#define ARROW_DOWN (char)129
#define ARROW_RIGHT (char)130
#define ARROW_LEFT (char)131

void console_server_main(void);
void console_read_notifer_main(void);

#endif

