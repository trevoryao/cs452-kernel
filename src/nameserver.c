#include "nameserver.h"

// lib
#include "uassert.h"
#include "msg.h"
#include "msg-type.h"
#include "task.h"
#include "util.h"

typedef struct nameserver_entry {
    char name[MAX_NAME_LENGTH];
    uint16_t tid;
    int storage_id;
    struct nameserver_entry *next;
} nameserver_entry;

typedef struct nameserver {
    nameserver_entry *hash_table[TABLE_SIZE];
    nameserver_entry storage[MAX_ENTRIES];
    uint16_t current_storage_id;
} nameserver;

static void nameserver_init(nameserver *ns) {
    ns->current_storage_id = 0;
    memset(ns->hash_table, 0, sizeof(nameserver_entry *) * TABLE_SIZE);
    memset(ns->storage, 0, sizeof(nameserver_entry) * MAX_ENTRIES);
}

// simple dbj2 hashing function (source: http://www.cse.yorku.ca/~oz/hash.html)
static int hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % TABLE_SIZE; // adapt hash to fit our table size
}

// If in storage: Overwrite in place -> if not append to list
static void nameserver_add_entry(nameserver *ns, char *name, uint16_t tid) {
    unsigned int hash_id = hash(name);

    nameserver_entry *last = NULL;
    nameserver_entry *curr = ns->hash_table[hash_id];

    while (curr != NULL) {
        // Just overwrite the tid in storage -> pointers can stay same
        if (!strncmp(name, ns->storage[curr->storage_id].name, MAX_NAME_LENGTH)) {
            curr->tid = tid;
            // we return as job is done after overwriting
            return;
        } else {
            last = curr;
            curr = curr->next;
        }
    }

    // save new entry in the storage
    strncpy(ns->storage[ns->current_storage_id].name, name, MAX_NAME_LENGTH);
    ns->storage[ns->current_storage_id].tid = tid;
    ns->storage[ns->current_storage_id].next = NULL;
    ns->storage[ns->current_storage_id].storage_id = ns->current_storage_id;

    // if last is NULL -> empty list -> we put it at the beginning
    if (last == NULL) {
        ns->hash_table[hash_id] = &ns->storage[ns->current_storage_id];
    } else {
        last->next = &ns->storage[ns->current_storage_id];
    }

    ns->current_storage_id += 1;
}

static int32_t nameserver_get_entry(nameserver *ns, char *name) {
    // hash the name
    unsigned int hash_id = hash(name);

    // look up and go through the list
    nameserver_entry *curr_entry = ns->hash_table[hash_id];

    while (curr_entry != NULL) {
        if (!strncmp(name, ns->storage[curr_entry->storage_id].name, MAX_NAME_LENGTH)) {
            return curr_entry->tid;
        }
        curr_entry = curr_entry->next;
    }

    return -1;
}

void nameserver_main() {
    // set up nameserver struct
    nameserver ns;

    // init the nameserver
    nameserver_init(&ns);

    // define a receive buffer
    struct msg_nameserver msg_received;
    int senderTid = 0;

    // define a response buffer
    struct msg_nameserver msg_reply;
    int response;

    // for loop
    for (;;) {
        // register to receive a message
        ULOG("nameserver - waiting for msg\r\n");
        response = Receive(&senderTid, (char *)&msg_received, sizeof(struct msg_nameserver));
        ULOG("nameserver - rcved msg %d\r\n", msg_received.type);

        // handle the received msg
        switch (msg_received.type) {
            case MSG_NAMESERVER_REGISTER: {
                nameserver_add_entry(&ns, (char *)msg_received.name, msg_received.tid);

                msg_reply.type = MSG_NAMESERVER_OK;
                msg_reply.tid = msg_received.tid;

                break;
            }
            case MSG_NAMESERVER_WHOIS: {
                int32_t answerTid = nameserver_get_entry(&ns, (char *)msg_received.name);

                msg_reply.type = (answerTid == -1) ?
                    MSG_NAMESERVER_ERROR : MSG_NAMESERVER_OK;
                msg_reply.tid = answerTid;

                break;
            }
            default: {
                msg_reply.type = MSG_NAMESERVER_ERROR;
                msg_reply.tid = msg_received.tid;

                break;
            }
        }

        // copy the name over
        strncpy(msg_reply.name, msg_received.name, MAX_NAME_LENGTH);

        // send the response msg
        response = Reply(senderTid, (char *)&msg_reply, sizeof(struct msg_nameserver));
        ULOG("nameserver - replying\r\n");

        // error handle
        switch (response) {
            case -1: ULOG("Requesting task-%d does not exist anymore\r\n", senderTid); break;
            case -2: ULOG("Requesting task-%d not waiting for reply anymore\r\n", senderTid); break;
            default: break;
        }
    }
}

int RegisterAs(const char *name) {
    uint16_t my_tid = MyTid();

    struct msg_nameserver msg;
    msg.type = MSG_NAMESERVER_REGISTER;
    msg.tid = my_tid;
    strncpy(msg.name, name, MAX_NAME_LENGTH);

    int ret = Send(NAMESERVER_TID, (char *)&msg, sizeof(struct msg_nameserver), (char *)&msg, sizeof(struct msg_nameserver));
    return (ret < 0 || msg.type == MSG_NAMESERVER_ERROR) ? -1 : 0;
}

int WhoIs(const char *name) {
    struct msg_nameserver msg;
    msg.type = MSG_NAMESERVER_WHOIS;
    strncpy(msg.name, name, MAX_NAME_LENGTH);

    int ret = Send(NAMESERVER_TID, (char *)&msg, sizeof(struct msg_nameserver), (char *)&msg, sizeof(struct msg_nameserver));
    return (ret < 0 || msg.type == MSG_NAMESERVER_ERROR) ? -1 : msg.tid;
}
