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
- Manages 
  - all current switch positions
  - all latest sensor activations
- Interface:
  - Change Switch
  - Change Train Speed
  - Wait for a specific sensor activation
    - (maybe wait for next sensor given current sensor & track position)
- Optional/TC2: possibly manage mutual exclusion

1. Train Worker Task - Trevor
- One task per train (speed, position)
- Manages the state of the train
  - expected position
  - actual position (based on next sensor activation)
  - Print Error function -> difference expected / actual position
- Route Planning
  - comes up with a routing/plan where do go

1. UI/Commands - Trevor
  - new commands/parsing for resetting track, constant speed, track positions
  - UI for displaying train velocities?

1. Planning Supervisor (TC2)
- Part of the Track Control Coordinator
  - Trains submit plans to Supervisor
  - Supervisor might adapt the plan / approve plan
  - Passes plan to Track Control Coordinator for execution
