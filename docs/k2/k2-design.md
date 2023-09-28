# K2 Design

Requirements from assignment spec:
1. Implement `Send()`, `Receive()`, and `Reply()`
    - Add the new kinds of block (see Kernel Description)
    - Adapt task descriptor for queue of tasks (copied, scheduler still keeps) waiting for msg response/etc
    - Possibly change scheduler algorithm to handle new types of blocked
    - Trevor
1. Message Data Structure
    - enum ident for type as first entry
    - create an args data structure for calling tasks
    - Trevor
1. Name Server
    - Created by the kernel (TID 1)
    - Data structure (self balancing tree? list? sorted array) for mapping names to TIDs
    - `RegisterAs` and `WhoIs` wrappers
    - Niclas
1. Game Server/Clients
    - Logging on all
    - Server: `Signup`, `Play`, `Quit` -- Niclas, implement interface
    - Client: Testing on the game server, hit all the branches -- Trevor, write up tests
1. Performance Measurement
    - As described, do it after the game server
    - Together, at the end
