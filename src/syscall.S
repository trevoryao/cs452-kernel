/* syscall handler */
    .text
    .global     syscall
    .type       syscall, %function

syscall:
    /* gcc moves args into w0 - w7 for us already */
    // bl      test_handle
    svc     #0  // sys call # on register, doesn't matter what we call with
    /* w0 holds return code after kernel switch back */
    ret
