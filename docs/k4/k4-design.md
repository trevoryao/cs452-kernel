# Kernel 4 Design

1. Setup UART Interrupts - Niclas/Trevor?
- Need events for Track TX (transmit), RX (receive), CTS (flow control), and Console RX
    - Add in `include/events.h`
- Augment kernel interrupts (`kernel/include/interrupts.h`) for GIC interrupt
    - add new InterruptID (153) to `INTERRUPT_IDS` array and increment `N_INTERRUPT_IDS`
    - interrupt triggered by actual event, so `trigger_interrupt` be ignored
    - augment `handle_interrupt` to read UART `MIS` registers (one per line) to determine
    which event triggered, and unblock waiting on event
        - possibly modify event_queue unblock f'ns to allow us to properly return things?
        - alternatively read from physical UART line in `includes/rpi.h`?
- disable UART FIFOs for Marklin line only

1. I/O Servers (and wrappers) - Niclas
- Create I/O server task, which receives **strings** (not just chars), and buffers them as individual chars. Separate queues for output/input.
    - Individual server task for each UART line, will want Marklin line to be the highest priority
    - new higher priority notifier level? notifier priority above server? test performance later
    - depending on which kind of server (console/marklin), waits on different interrupts
- Each queue has a notifier, which actually waits on the interrupts, structured the same way:
```
for (;;) {
    if (able to perform action?) {
        do_action();
    } else {
        AwaitEvent(action);
    }
}
```
    - Console output: regular blocking output calls
    - Console input: polling every N ms for input, and pulling it all in and buffering it, returning to server, who can return char by char on request to user tasks
    - Marklin input: wait on marklin RX
    - Marklin output case special: need to implement the finite state machine as described
```
for (;;) {
    while (CTS up) {
        if (can send byte) {
            send_byte();
        } else {
            AwaitEvent(marklin_TX);
        }
    }

    AwaitEvent(CTS);
}
```
- For wrappers, can drop `channel` param, since each server task is specific to a channel
- Recreate usual printf f'ns (`putl`, `puts`, `printf`) with the specified `getc`/`putc`
- Test with I/O test, output onto lines

1. porting A0 - Trevor
- Need tasks which are each responsible for one thing:
    - updating the displayed system clock
    - sending & receiving sensor data (& displaying it)
    - receiving user input
    - processing user input & output to track
        - reverse command will need to create new task to wait on reverse to finish
    - sensor task and user output tasks will need to "ping pong" between each other by passing messages when the other is not currently running