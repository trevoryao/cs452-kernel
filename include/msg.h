#ifndef __MSG_H__
#define __MSG_H__

/*
 * Sends a message to another task and receives a reply.
 * The message, in a buffer in the sending task’s memory,
 * is copied to the memory of the task to which it is sent
 * by the kernel. Send() supplies a buffer into which the
 * reply is to be copied, and the size of the reply buffer,
 * so that the kernel can detect overflow. When Send()
 * returns without error it is guaranteed that the message
 * has been received, and that a reply has been sent, not
 * necessarily by the same task. The kernel will not overflow
 * the reply buffer. If the size of the reply set exceeds
 * rplen, the reply is truncated and the buffer contains the
 * first rplen characters of the reply. The caller is expected
 * to check the return value and to act accordingly. There is
 * no guarantee that Send() will return. If, for example, the
 * task to which the message is directed never calls Receive(),
 * Send() never returns and the sending task remains blocked
 * forever. Send() has a passing resemblance, and no more, to
 * a remote procedure call.
 *
 * Return Value:
 *  - >=0	the size of the message returned by the replying
 *          task. The actual reply is less than or equal to
 *          the size of the reply buffer provided for it.
 *          Longer replies are truncated.
 *  - -1	tid is not the task id of an existing task.
 *  - -2	send-receive-reply transaction could not be completed.
 */
int Send(int tid, const char *msg, int msglen, char *reply, int rplen);

/*
 * If there are no unreceived Sends for this caller,
 * Receive blocks until a message is Sent to the caller.
 * Receive returns with the sent message in its message buffer
 * and tid set to the task id of the task that sent the message.
 * The argument msg must point to a buffer at least as large as
 * msglen. The kernel will not overflow the message buffer. If
 * the size of the incoming message set exceeds msglen, the
 * message is truncated and the buffer contains the first msglen
 * characters of the message sent. The caller is expected to
 * check the return value and to act accordingly.
 *
 * Return Value:
 *  - >=0	the size of the message sent by the sender (stored
 *          in tid). The actual message is less than or equal
 *          to the size of the message buffer supplied. Longer
 *          messages are truncated.
 */
int Receive(int *tid, char *msg, int msglen);

/*
 * Sends a reply to a task that previously sent a message.
 * When it returns without error, the reply has been copied
 * into the sender’s memory. The calling task and the sender
 * return at the same logical time, so whichever is of higher
 * priority runs first.
 *
 * Return Value:
 *  - >=0	the size of the reply message transmitted to the
 *          original sender task. If this is less than the size
 *          of the reply message, the message has been truncated.
 *  - -1	tid is not the task id of an existing task.
 *  - -2	tid is not the task id of a reply-blocked task.
 */
int Reply(int tid, const char *reply, int rplen);

#endif
