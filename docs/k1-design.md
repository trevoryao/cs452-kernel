# K1 Design

Requirements from Assignment Spec:
1. Context switch to/from user-level task & kernel -> main problem
    - use raw assembly to copy between registers and "structured arg"
    - need to play around with C struct spacing
    - Trevor does (more familiar with ASM)?
3. Request mechanism for providing args and returning results
    - Use syscalls -> exceptions (built in to aarch64 with `svc N` instruction)
        - `N` stored in ESR_ELn
    - Pass args with x0, ..., x7; return from x0
    - Exception Link Register (ELR) for kernel back to task
    - Saved Process Status Register (SPSR)
    - Jump to (another/same) task with `ERET` instruction
        - Context switch from above
    - Trevor does (more familar with ASM)
2. Collection of Task Descriptors
    - slab allocation: dedicated memory region for each relevant data type
        - stored freed objects in free list, use intrusive overlay to manage
            - pointer casting to switch between structured and raw view
            - Alt use union
        - allocate first from free list, then from slab
    - No `malloc`!
    - Niclas (need collaboration with task_t obj)
4. PQs for scheduling
    - use intrusive links in task objs -- array (of all priorities) of FIFO linked lists
    - Niclas
5. Kernel algos for manipulating task descriptors and ready queues
    - Covered by 2 & 4
