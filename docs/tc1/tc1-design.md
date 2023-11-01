# TC 1 Design

1. Marklin Server (remains unchanged)
- handles sending and receiving to/from marklin

1. Sensor Reading Task/Notifier - Niclas
- Waits for a certain time period
- Checks if queue at Marklin server is empty
  - send sensor reading command
  - parses the sensor activations
-> ONLY if sensor have been tripped -> send it to the track control server (with timestamp)

1. Track Control Coordinator (Server) -> manages state of the track - Niclas
- Manages:
  - all current switch positions
  - all latest sensor activations
- Interface:
  - Change Switch
  - Change Train Speed
  - Wait for a specific sensor activation
    - (maybe wait for next sensor given current sensor & track position)
- Optional/TC2: possibly manage mutual exclusion
  - Manage graph representation of the track, set infinite weights to enforce mutual exclusion?
  - slow/speed-up trains

1. Train Worker Task - Trevor
- One task per train (speed, position)
- Manages the state of the train
  - expected position
  - actual position (based on next sensor activation)
  - Print Error function -> difference expected / actual position
- Task is a Server, with:
  - notifier for waiting on sensor "interrupts"
  - notifier for waiting on timer "interrupts"
  - worker for calculating route (route planner)
    - modified Dijkstra's Algorithm (w/out backtracking)
    - on initialisation (i.e. given a destination), calculates the total stopping distance (spd -> 7 -> 0) via (stopping distance + accel, v1, v2)
    - for each "iteration" of Dijkstra, search at most 2 * total_stopping_distance
      - for each "decision point sensor" (throw a switch, stopping, acceleration finished) found, store in a message with the sensor number and action
      - send message to the Task Server to buffer descision points (context switch away)
  - server responsibilities
    - for each decision point sensor in the queue, wait for said sensor to trip via notifier, and then send action to train control coordinator
    - if no such decision point sensor, wait for the next sensor activation and hope the route planner has caught up

1. UI/Commands - Trevor
  - new commands/parsing for resetting track, constant speed, track positions
  - UI for displaying sensor predictions
  - UI for displaying idle
    - new syscalls

1. Planning Supervisor (TC2)
- Part of the Track Control Coordinator
  - Trains submit plans to Supervisor
  - Supervisor might adapt the plan / approve plan
  - Passes plan to Track Control Coordinator for execution
