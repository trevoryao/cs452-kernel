# Kernel 3 Design

Requirements from assignment spec:
1. setup kernel to handle interrupts
    - modify kernel_init to initialise interrupts as described in manual
    - turn off user-task interrupt masking
    - modifications to `context_switch_out`?
    - Trevor
1. an implementation of the `AwaitEvent()` system call,
    - see above
    - add blocking as part of that
    - add linked list node for blocked tasks
    - see Week 5 notes for implementation details
    - Trevor
1. an implementation of an idle task,
    - modify kernel to track time idle task was active, and total active time on exit, print
    - dependent on the clock server interior `time()` method (i.e. not the Send wrapper, but the actual register access)
1. implementations of a clock server and a clock notifier
    - need to copy separate time getter class - Trevor
    - blocking notifier, receiving server
    - modify kernel_init to start on start-up
    - Niclas
1. implementations of Time(), Delay(), and DelayUntil() as wrappers for Send() to the clock server.
    - self explanatory
    - Niclas
1. an implementation of clock server client tasks, for testing.
    - see spec
    - Niclas
