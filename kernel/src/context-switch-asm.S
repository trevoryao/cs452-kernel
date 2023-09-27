/*
 * Code for performing context switch from user task ->
 * kernel task and vice versa
 */

    .text

    .global init_exception_handlers
    .type   init_exception_handlers, %function

    .global exception_vector
    .type   exception_vector, %common

    .global context_switch_in
    .type   context_switch_in, %function

    .global context_switch_out
    .type   context_switch_out, %function

    .global branch_if_assert
    .type   branch_if_assert, %function

    .global get_el
    .type   get_el, %function

    .globl unimplemented_handler
    .globl reboot


// Aarch64 exception table (see Arm Programmer's Guide 10.4
// for more info).
.balign 2048
exception_vector:
    /* Current EL with SP0 */
    curr_sp0_sync:
        adr x0, curr_sp0_sync       // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    curr_sp0_irq:
        adr x0, curr_sp0_irq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    curr_sp0_fiq:
        adr x0, curr_sp0_fiq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    curr_sp0_serror:
        adr x0, curr_sp0_serror     // pass current PC as arg
        bl  unimplemented_handler   // exits
        b   reboot

    /* Current EL with SPn */
    .balign 0x80
    curr_spx_sync:
        bl      branch_if_assert    // if BRK instruction (assert failed), restarts

        adr     x0, curr_spx_sync   // pass current PC as arg
        mrs     x1, ESR_EL1
        bl      unimplemented_handler // exits
        b       reboot


    .balign 0x80
    curr_spx_irq:
        adr x0, curr_spx_irq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    curr_spx_fiq:
        adr x0, curr_spx_fiq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    curr_spx_serror:
        adr x0, curr_spx_serror     // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    /* Lower EL using AArch64 */
    .balign 0x80
    lower_64_sync:
        bl  branch_if_assert        // if user-level break, will branch (failed assert)
        b   context_switch_in       // otherwise attempt to handle as syscall

    .balign 0x80
    lower_64_irq:
        adr x0, lower_64_irq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    lower_64_fiq:
        adr x0, lower_64_fiq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    lower_64_serror:
        adr x0, lower_64_serror     // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    /* Lower EL using AArch32 */
    .balign 0x80
    lower_32_sync:
        adr x0, lower_32_sync       // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    lower_32_irq:
        adr x0, lower_32_irq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    lower_32_fiq:
        adr x0, lower_32_fiq        // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

    .balign 0x80
    lower_32_serror:
        adr x0, lower_32_serror     // pass current PC as arg
        mrs x1, ESR_EL1
        bl  unimplemented_handler   // exits
        b   reboot

.balign 4
init_exception_handlers:
    /* set VBAR_EL1 */
    ldr     x9, =exception_vector
    msr     VBAR_EL1, x9
    ret


context_switch_out:
    // Save Kernel state to kernel_task
    // using sp1
    str     x0, [sp, #-8]!      // store for later (curr_user_task)
    str     x1, [sp, #-8]!      // store for later (kernel_task)

    mov     x0, x1              // kernel_task

    str     x1, [x0, #8]        // x1
    str     x2, [x0, #16]       // x2
    str     x3, [x0, #24]       // x3
    str     x4, [x0, #32]       // x4
    str     x5, [x0, #40]       // x5
    str     x6, [x0, #48]       // x6
    str     x7, [x0, #56]       // x7
    str     x8, [x0, #64]       // x8
    str     x9, [x0, #72]       // x9
    str     x10, [x0, #80]      // x10
    str     x11, [x0, #88]      // x11
    str     x12, [x0, #96]      // x12
    str     x13, [x0, #104]     // x13
    str     x14, [x0, #112]     // x14
    str     x15, [x0, #120]     // x15
    str     x16, [x0, #128]     // x16
    str     x17, [x0, #136]     // x17
    str     x18, [x0, #144]     // x18
    str     x19, [x0, #152]     // x19
    str     x20, [x0, #160]     // x20
    str     x21, [x0, #168]     // x21
    str     x22, [x0, #176]     // x22
    str     x23, [x0, #184]     // x23
    str     x24, [x0, #192]     // x24
    str     x25, [x0, #200]     // x25
    str     x26, [x0, #208]     // x26
    str     x27, [x0, #216]     // x27
    str     x28, [x0, #224]     // x28
    str     x29, [x0, #232]     // x29
    str     x30, [x0, #240]     // x30

    ldr     x1, [sp, #8]        // saved x0 value
    str     x1, [x0]            // x0

    // Load user task state from curr_user_task into registers
    ldr     x0, [sp, #8]        // curr_user_task

    ldr     x2, [x0, #16]       // x2
    ldr     x3, [x0, #24]       // x3
    ldr     x4, [x0, #32]       // x4
    ldr     x5, [x0, #40]       // x5
    ldr     x6, [x0, #48]       // x6
    ldr     x7, [x0, #56]       // x7
    ldr     x8, [x0, #64]       // x8
    ldr     x9, [x0, #72]       // x9
    ldr     x10, [x0, #80]      // x10
    ldr     x11, [x0, #88]      // x11
    ldr     x12, [x0, #96]      // x12
    ldr     x13, [x0, #104]     // x13
    ldr     x14, [x0, #112]     // x14
    ldr     x15, [x0, #120]     // x15
    ldr     x16, [x0, #128]     // x16
    ldr     x17, [x0, #136]     // x17
    ldr     x18, [x0, #144]     // x18
    ldr     x19, [x0, #152]     // x19
    ldr     x20, [x0, #160]     // x20
    ldr     x21, [x0, #168]     // x21
    ldr     x22, [x0, #176]     // x22
    ldr     x23, [x0, #184]     // x23
    ldr     x24, [x0, #192]     // x24
    ldr     x25, [x0, #200]     // x25
    ldr     x26, [x0, #208]     // x26
    ldr     x27, [x0, #216]     // x27
    ldr     x28, [x0, #224]     // x28
    ldr     x29, [x0, #232]     // x29
    ldr     x30, [x0, #240]     // x30

    ldr     x1, [x0, #248]      // pc
    msr     ELR_EL1, x1

    ldr     x1, [x0, #256]      // sp
    msr     SP_EL0, x1

    ldr     w1, [x0, #264]      // pstate (32 bits)
    msr     SPSR_EL1, x1

    ldr     x1, [x0, #8]        // x1
    ldr     x0, [x0]            // x0

    msr     SPSel, #0           // switch SP (just incase)
    eret                        // return to EL0


// saves user context to "curr_task" and loads kernel context in
// returns syscall number when switched back
context_switch_in:
    // Write registers to curr_user_task
    // Using EL1 SP already

    msr     SPSel, #1           // make sure SP is EL1

    str     x0, [sp, #-8]!      // store for later

    ldr     x0, [sp, #16]       // curr_user_task

    str     x1, [x0, #8]        // x1
    str     x2, [x0, #16]       // x2
    str     x3, [x0, #24]       // x3
    str     x4, [x0, #32]       // x4
    str     x5, [x0, #40]       // x5
    str     x6, [x0, #48]       // x6
    str     x7, [x0, #56]       // x7
    str     x8, [x0, #64]       // x8
    str     x9, [x0, #72]       // x9
    str     x10, [x0, #80]      // x10
    str     x11, [x0, #88]      // x11
    str     x12, [x0, #96]      // x12
    str     x13, [x0, #104]     // x13
    str     x14, [x0, #112]     // x14
    str     x15, [x0, #120]     // x15
    str     x16, [x0, #128]     // x16
    str     x17, [x0, #136]     // x17
    str     x18, [x0, #144]     // x18
    str     x19, [x0, #152]     // x19
    str     x20, [x0, #160]     // x20
    str     x21, [x0, #168]     // x21
    str     x22, [x0, #176]     // x22
    str     x23, [x0, #184]     // x23
    str     x24, [x0, #192]     // x24
    str     x25, [x0, #200]     // x25
    str     x26, [x0, #208]     // x26
    str     x27, [x0, #216]     // x27
    str     x28, [x0, #224]     // x28
    str     x29, [x0, #232]     // x29
    str     x30, [x0, #240]     // x30

    ldr     x1, [sp], #8        // saved x0 value
    str     x1, [x0]            // x0

    mrs     x1, ELR_EL1
    str     x1, [x0, #248]      // pc

    mrs     x1, SP_EL0
    str     x1, [x0, #256]      // sp

    mrs     x1, SPSR_EL1
    str     w1, [x0, #264]      // pstate (32 bit)

    // write kernel registers in from kernel_task
    ldr     x0, [sp]            // kernel_task

    ldr     x1, [x0, #8]        // x1
    ldr     x2, [x0, #16]       // x2
    ldr     x3, [x0, #24]       // x3
    ldr     x4, [x0, #32]       // x4
    ldr     x5, [x0, #40]       // x5
    ldr     x6, [x0, #48]       // x6
    ldr     x7, [x0, #56]       // x7
    ldr     x8, [x0, #64]       // x8
    ldr     x9, [x0, #72]       // x9
    ldr     x10, [x0, #80]      // x10
    ldr     x11, [x0, #88]      // x11
    ldr     x12, [x0, #96]      // x12
    ldr     x13, [x0, #104]     // x13
    ldr     x14, [x0, #112]     // x14
    ldr     x15, [x0, #120]     // x15
    ldr     x16, [x0, #128]     // x16
    ldr     x17, [x0, #136]     // x17
    ldr     x18, [x0, #144]     // x18
    ldr     x19, [x0, #152]     // x19
    ldr     x20, [x0, #160]     // x20
    ldr     x21, [x0, #168]     // x21
    ldr     x22, [x0, #176]     // x22
    ldr     x23, [x0, #184]     // x23
    ldr     x24, [x0, #192]     // x24
    ldr     x25, [x0, #200]     // x25
    ldr     x26, [x0, #208]     // x26
    ldr     x27, [x0, #216]     // x27
    ldr     x28, [x0, #224]     // x28
    ldr     x29, [x0, #232]     // x29
    ldr     x30, [x0, #240]     // x30

    // all registers but x0
    ldr     x0, [x0]            // x0

    add     sp, sp, #16         // pop tasks off the stack

    br      x30                 // jump back to caller (task_activate)


// returns current exception level
get_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2          // bits 2 & 3
    and     x0, x0, #0x3        // mask
    ret


branch_if_assert:
    // check for failed assert
    mrs     x0, ESR_EL1
    lsr     x0, x0, #26
    and     x0, x0, #0x3F       // extract EC
    sub     x0, x0, #0x3C       // EC is break?
    cbz     x0, reboot          // if breakpoint, restart
    ret                         // otherwise return
