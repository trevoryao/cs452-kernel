#include "uart-server.h"
#include "marklin-server.h"
#include "console-server.h"
#include "task.h"
#include "nameserver.h"
#include "msg.h"
#include "rpi.h"
#include "clock-server.h"

void user_main(void) {
    // Startup marklin
    //Create(P_SERVER, marklin_server_main);
    //int marklin_tid = WhoIs(MARKLIN_SERVER_NAME);

    // Startup clockserver
    Create(P_SERVER, clockserver_main);


    // Startup console
    Create(P_SERVER, console_server_main);
    int console_tid = WhoIs(CONSOLE_SERVER_NAME);


    // Test to print sth
    uart_printf(CONSOLE, "------Starting Test - Server should be running-----\r\n");

    Printf(console_tid, "This is a number 10: %d and this should be a c: %c \r\n", 10, 'c');


    // write sth to the marklin server
    //Puts(marklin_tid, "\x08\x18\x08\x4D");
    /*
    Puts(marklin_tid, 10);
    Putc(marklin_tid, 24);
    Putc(marklin_tid, 10);
    Putc(marklin_tid, 77);
*/
    for(;;) {
        char c = Getc(console_tid);
        uart_printf(CONSOLE, "received character: %c \r\n", c);
    }
}