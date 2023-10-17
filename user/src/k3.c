#include "k3.h"

#include <stdint.h>
#include "util.h"
#include "clock.h"
#include "task.h"
#include "clock-server.h"
#include "msg.h"
#include "nameserver.h"
#include "uassert.h"

struct clock_client_msg {
    int delayInterval;
    int no_of_delays;
    int clientNumber;
};


void clock_client() {
    struct clock_client_msg task;
    int tid = MyTid();
    int p_tid;

    // get clockserver tid
    int clockserver_tid = WhoIs("clockserver");

    Receive(&p_tid, (char *)&task, sizeof(struct clock_client_msg));
    ULOG("[Clock Client] %d -- received info from parent\r\n", task.clientNumber);
    Reply(p_tid, NULL, 0); // unblock parent

    for (int i = 0; i < task.no_of_delays; i++) {
        Delay(clockserver_tid, task.delayInterval);
        uart_printf(CONSOLE, "Client tid %d: delay interval %d (%d)\r\n", tid, task.delayInterval, i + 1);
    }
}

void kernel3_test() {
    // Create Clock server
    Create(P_SERVER, clockserver_main);

    ULOG("[User Task] Clockserver started - starting clients\r\n");

    // create 4 user tasks with expected behavior
    uint16_t client1 = Create(P_VHIGH, clock_client);
    uint16_t client2 = Create(P_HIGH, clock_client);
    uint16_t client3 = Create(P_MED, clock_client);
    uint16_t client4 = Create(P_LOW, clock_client);

    struct clock_client_msg task_description;

    // sending the message to the client 1
    task_description.clientNumber = 1;
    task_description.delayInterval = 10;
    task_description.no_of_delays = 20;
    Send(client1, (char *)&task_description, sizeof(struct clock_client_msg),
    (char *)&task_description, sizeof(struct clock_client_msg));

    // sending the message to the client 2
    task_description.clientNumber = 2;
    task_description.delayInterval = 23;
    task_description.no_of_delays = 9;
    Send(client2, (char *)&task_description, sizeof(struct clock_client_msg),
    (char *)&task_description, sizeof(struct clock_client_msg));

    // sending the message to the client 2
    task_description.clientNumber = 3;
    task_description.delayInterval = 33;
    task_description.no_of_delays = 6;
    Send(client3, (char *)&task_description, sizeof(struct clock_client_msg),
    (char *)&task_description, sizeof(struct clock_client_msg));

    // sending the message to the client 4
    task_description.clientNumber = 4;
    task_description.delayInterval = 71;
    task_description.no_of_delays = 3;
    Send(client4, (char *)&task_description, sizeof(struct clock_client_msg),
    (char *)&task_description, sizeof(struct clock_client_msg));

}