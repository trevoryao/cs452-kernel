#ifndef __NAMESERVER_H__
#define __NAMESERVER_H__

#include <stdint.h>

#define TABLE_SIZE 64
#define MAX_ENTRIES 256
#define MAX_NAME_LENGTH 256

#define NAMESERVER_TID 1

void nameserver_main();

/*
 * Registers the task id of the caller under the given name.
 * On return without error it is guaranteed that all WhoIs()
 * calls by any task will return the task id of the caller
 * until the registration is overwritten. If another task has
 * already registered with the given name, its registration
 * is overwritten.
 *
 * Return Value:
 *  - 0	    success.
 *  - -1	unable to reach name server
 */
int RegisterAs(const char *name);

/*
 * Asks the name server for the task id of the task that is
 * registered under the given name. Whether WhoIs() blocks
 * waiting for a registration or returns with an error, if no
 * task is registered under the given name, is implementation-
 * dependent. There is guaranteed to be a unique task id associated
 * with each registered name, but the registered task may change
 * at any time after a call to WhoIs().
 *
 * Return Value:
 *  - tid	task id of the registered task.
 *  -1	    unable to reach name server
 */
int WhoIs(const char *name);

#endif
