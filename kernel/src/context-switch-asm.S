/*
 * Code for performing context switch from user task ->
 * kernel task and vice versa
 */

    .text

    .global set_context_switch_handler
    .type   set_context_switch_handler, %function

    .global exception_vector
    .type   exception_vector, %common

    .global context_switch_in
    .type   context_switch_in, %function

    .global context_switch_out
    .type   context_switch_out, %function

    .global get_el
    .type   get_el, %function

    .globl unimplemented_handler

    .globl kernel_task
    .globl curr_user_task


// Aarch64 exception table (see Arm Programmer's Guide 10.4
// for more info).
.balign 2048
exception_vector:
    /* Current EL with SP0 */
    curr_sp0_sync:
        adr x0, curr_sp0_sync     // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_sp0_irq:
        adr x0, curr_sp0_irq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_sp0_fiq:
        adr x0, curr_sp0_fiq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_sp0_serror:
        adr x0, curr_sp0_serror    // pass current PC as arg
        b   unimplemented_handler  // exits

    /* Current EL with SPn */
    .balign 0x80
    curr_spx_sync:
        adr x0, curr_spx_sync     // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_spx_irq:
        adr x0, curr_spx_irq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_spx_fiq:
        adr x0, curr_spx_fiq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    curr_spx_serror:
        adr x0, curr_spx_serror   // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    /* Lower EL using AArch64 */
    .balign 0x80
    lower_64_sync:
        b   context_switch_in    /* SVC from EL0 */

    .balign 0x80
    lower_64_irq:
        adr x0, lower_64_irq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    lower_64_fiq:
        adr x0, lower_64_fiq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    lower_64_serror:
        adr x0, lower_64_serror   // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    /* Lower EL using AArch32 */
    .balign 0x80
    lower_32_sync:
        adr x0, lower_32_sync     // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    lower_32_irq:
        adr x0, lower_32_irq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    lower_32_fiq:
        adr x0, lower_32_fiq      // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

    .balign 0x80
    lower_32_serror:
        adr x0, lower_32_serror   // pass current PC as arg
        mrs x1, ESR_EL1
        b   unimplemented_handler // exits

.balign 4
init_exception_handlers:
    /* set VBAR_EL1 */
    ldr     x9, =exception_vector
    msr     VBAR_EL1, x9
    ret

// saves user context to "curr_task" and loads kernel context in
// returns syscall number when switched back
context_switch_in:
    // Write registers to curr_user_task
    // Using EL1 SP already

    msr     SPSel, #1           // make sure SP is EL1

    str     x9, [sp, #-8]!      // store for later

    adr     x9, curr_user_task  // x9 -> &curr_user_task
    ldr     x9, [x9, 0]         // x9 -> curr_user_task

    str     x0, [x9, #0]        // x0
    str     x1, [x9, #8]        // x1
    str     x2, [x9, #16]       // x2
    str     x3, [x9, #24]       // x3
    str     x4, [x9, #32]       // x4
    str     x5, [x9, #40]       // x5
    str     x6, [x9, #48]       // x6
    str     x7, [x9, #56]       // x7
    str     x8, [x9, #64]       // x8
    str     x10, [x9, #80]      // x10
    str     x11, [x9, #88]      // x11
    str     x12, [x9, #96]      // x12
    str     x13, [x9, #104]     // x13
    str     x14, [x9, #112]     // x14
    str     x15, [x9, #120]     // x15
    str     x16, [x9, #128]     // x16
    str     x17, [x9, #136]     // x17
    str     x18, [x9, #144]     // x18
    str     x19, [x9, #152]     // x19
    str     x20, [x9, #160]     // x20
    str     x21, [x9, #168]     // x21
    str     x22, [x9, #176]     // x22
    str     x23, [x9, #184]     // x23
    str     x24, [x9, #192]     // x24
    str     x25, [x9, #200]     // x25
    str     x26, [x9, #208]     // x26
    str     x27, [x9, #216]     // x27
    str     x28, [x9, #224]     // x28
    str     x29, [x9, #232]     // x29
    str     x30, [x9, #240]     // x30

    ldr     x10, [sp], #8       // saved x9 value
    str     x10, [x9, #72]      // x9

    mrs     x10, ELR_EL1
    str     x10, [x9, #248]     // pc

    mrs     x10, SP_EL0
    str     x10, [x9, #256]     // sp

    mrs     x10, SPSR_EL1
    str     x10, [x9, #264]     // pstate

    // write kernel registers in from kernel_task
    adr     x1, kernel_task     // x1 -> &kernel_task
    ldr     x1, [x1, 0]         // x1 -> kernel_task

    str     x1, [sp, #-8]!       // store for later

    ldr     x0, [x1, #0]        // x0
    ldr     x2, [x1, #16]       // x2
    ldr     x3, [x1, #24]       // x3
    ldr     x4, [x1, #32]       // x4
    ldr     x5, [x1, #40]       // x5
    ldr     x6, [x1, #48]       // x6
    ldr     x7, [x1, #56]       // x7
    ldr     x8, [x1, #64]       // x8
    ldr     x9, [x9, #72]       // x9
    ldr     x10, [x1, #80]      // x10
    ldr     x11, [x1, #88]      // x11
    ldr     x12, [x1, #96]      // x12
    ldr     x13, [x1, #104]     // x13
    ldr     x14, [x1, #112]     // x14
    ldr     x15, [x1, #120]     // x15
    ldr     x16, [x1, #128]     // x16
    ldr     x17, [x1, #136]     // x17
    ldr     x18, [x1, #144]     // x18
    ldr     x19, [x1, #152]     // x19
    ldr     x20, [x1, #160]     // x20
    ldr     x21, [x1, #168]     // x21
    ldr     x22, [x1, #176]     // x22
    ldr     x23, [x1, #184]     // x23
    ldr     x24, [x1, #192]     // x24
    ldr     x25, [x1, #200]     // x25
    ldr     x26, [x1, #208]     // x26
    ldr     x27, [x1, #216]     // x27
    ldr     x28, [x1, #224]     // x28
    ldr     x29, [x1, #232]     // x29
    ldr     x30, [x1, #240]     // x30

    // all registers but x1
    ldr     x1, [sp], #8

    br      x30                 // jump back to caller (task_activate)

context_switch_out:
    // Save Kernel state to kernel_task
    // using sp1
    str     x1, [sp, #-8]!      // store for later

    adr     x1, kernel_task     // x1 -> &kernel_task
    ldr     x1, [x1, 0]         // x1 -> kernel_task

    str     x0, [x1, #0]        // x0
    str     x2, [x1, #16]       // x2
    str     x3, [x1, #24]       // x3
    str     x4, [x1, #32]       // x4
    str     x5, [x1, #40]       // x5
    str     x6, [x1, #48]       // x6
    str     x7, [x1, #56]       // x7
    str     x8, [x1, #64]       // x8
    str     x9, [x1, #72]       // x9
    str     x10, [x1, #80]      // x10
    str     x11, [x1, #88]      // x11
    str     x12, [x1, #96]      // x12
    str     x13, [x1, #104]     // x13
    str     x14, [x1, #112]     // x14
    str     x15, [x1, #120]     // x15
    str     x16, [x1, #128]     // x16
    str     x17, [x1, #136]     // x17
    str     x18, [x1, #144]     // x18
    str     x19, [x1, #152]     // x19
    str     x20, [x1, #160]     // x20
    str     x21, [x1, #168]     // x21
    str     x22, [x1, #176]     // x22
    str     x23, [x1, #184]     // x23
    str     x24, [x1, #192]     // x24
    str     x25, [x1, #200]     // x25
    str     x26, [x1, #208]     // x26
    str     x27, [x1, #216]     // x27
    str     x28, [x1, #224]     // x28
    str     x29, [x1, #232]     // x29
    str     x30, [x1, #240]     // x30

    ldr     x10, [sp], #8       // saved x1 value
    str     x10, [x1, #8]       // x1

    // Load user task state from curr_user_task into registers
    adr     x9, curr_user_task  // x9 -> &curr_user_task
    ldr     x9, [x9, 0]         // x9 -> curr_user_task

    ldr     x10, [x9, #72]      // x9
    str     x10, [sp, #-8]!      // store for later

    ldr     x10, [x9, #80]      // x10
    str     x10, [sp, #-8]!     // store for later

    ldr     x0, [x9, #0]        // x0
    ldr     x1, [x9, #8]        // x1
    ldr     x2, [x9, #16]       // x2
    ldr     x3, [x9, #24]       // x3
    ldr     x4, [x9, #32]       // x4
    ldr     x5, [x9, #40]       // x5
    ldr     x6, [x9, #48]       // x6
    ldr     x7, [x9, #56]       // x7
    ldr     x8, [x9, #64]       // x8
    ldr     x11, [x9, #88]      // x11
    ldr     x12, [x9, #96]      // x12
    ldr     x13, [x9, #104]     // x13
    ldr     x14, [x9, #112]     // x14
    ldr     x15, [x9, #120]     // x15
    ldr     x16, [x9, #128]     // x16
    ldr     x17, [x9, #136]     // x17
    ldr     x18, [x9, #144]     // x18
    ldr     x19, [x9, #152]     // x19
    ldr     x20, [x9, #160]     // x20
    ldr     x21, [x9, #168]     // x21
    ldr     x22, [x9, #176]     // x22
    ldr     x23, [x9, #184]     // x23
    ldr     x24, [x9, #192]     // x24
    ldr     x25, [x9, #200]     // x25
    ldr     x26, [x9, #208]     // x26
    ldr     x27, [x9, #216]     // x27
    ldr     x28, [x9, #224]     // x28
    ldr     x29, [x9, #232]     // x29
    ldr     x30, [x9, #240]     // x30

    ldr     x10, [x9, #248]     // pc
    msr     ELR_EL1, x10

    ldr     x10, [x9, #256]     // sp
    msr     SP_EL0, x10

    ldr     x10, [x9, #264]     // pstate
    msr     SPSR_EL1, x10

    ldr     x10, [sp], #8       // saved x10
    ldr     x9, [sp], #8        // saved x9

    msr     SPSel, #0           // switch SP (just incase)

    eret                        // return to EL0


// returns current exception level
get_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2          // bits 2 & 3
    and     x0, x0, #0x3        // mask
    ret
