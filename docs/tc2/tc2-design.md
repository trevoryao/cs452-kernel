# TC2 Design/Roadmap

## Things to Improve from TC1
1. Sensor Predictions at non-constant speed
1.

## Requirements for TC2
1. **Route Finding**: You must implement route finding, which provides a train with a route to a given destination. The space of routes considered by your route finder should include routes that require the train to change direction. There are many more routes including direction changes than there are without, which is important to you for two reasons. When you include the possibility of a direction change, you can often find a much shorter route to a destination. With shorter routes, multiple trains have a higher chance for fewer overlapping sections in the their routing.
- Ask Ken if this includes reversing mid-route.

1. **Sensor Attribution**: When two or more trains are moving simultaneously, each sensor report is either spurious or caused by one of the trains. Your implementation should consistently attribute sensor reports to the correct train.
- Requires better sensor predictions at acceleration/stopping (i.e. modelling)

1. **Collision Avoidance**: Train collisions should be avoided. This requires you to define a policy that makes it impossible for two trains to occupy the same location at the same time, and to implement your policy. You can avoid collisions in the time domain (slowing, pausing) and/or space domain (alternative path). However, trivial non-collision algorithms, such as only running one train at a time, are not acceptable.

1. **Robustness**: Your application is expected to be robust against a 'reasonable' level of errors in sensor reports and changes in the state of the turn-outs. In addition, it should avoid typical problems, such as switching a turnout while a train is passing. The route finder should be able to respond to changes in track configuration caused by unavailability of track segments. Train control should drive a train fast enough to not get stuck.
- Attributing sensor predictions better, and implementing timeouts if a sensor prediction is late (I.e. never comes), assume within _some_ window that it was just missed
    - if miss (3 in a row, abort)
- Add flags in the routing to disable certain segments of the track

## Gameplan
- Non-constant speed positioning [Tues 14 Nov]
    - sensor attribution to train, timeouts on sensor activation, detect abort
    - change waiting queue to include train information
- Track Server with TC1 (i.e. constant re-computation of our route using a track server) [Thurs 16 Nov]
    - add segment locking
        - a segment is a contiguous span of track where there are no branches
- Test with single train running on TC1 interface [Fri 17 Nov]
- Create method based on the modelling (i.e. constant/non-constant speed) running on worker task [Sun 19 Nov]
of TCC to check if there is a collision between any two given trains on the track
    - collision detection is based on buffer window (example but adjustable 100mm) between trains
    - if there is a collision, notify trains to adjust speed, take different route
    - TCC has final authority to emergency-stop (i.e. SPD_REVERSE) train(s) if detecting VERY IMMINENT collision
- mid-route reversing [Mon 20 Nov]
- documentation [Tues 21 Nov]

## Gameplan 2
**As of [Tues 14 Nov]**
- Non-constant speed position [Wed 15 Nov]
    - Recalibrate to use SPD_MED/11 as our base speed, acceleration straight to SPD_MED
        - change routing algorithm
    - Test non-zero speed change by altering routing algorithm speed change time
    - Merge and integrate with sensor timeout branch
        - add ability to change sensor expectation to none on the fly (in case of sudden stopping, etc)
        - Make sure TCC takes new predictions into account when made by positioning (i.e. change on the fly)
- Integrating Locking into TC1 [Thurs 16 Nov]
    - Add constant route re-computation
        - Calibration for short moves?
    - Random locking? start with a segment locked
- Collision Avoidance [Sun 19 Nov]
    - via Locking
        - multiple sectors?
    - via detecting different train sensor timeout overlap
    - Add better mid-route stopping into routing algorithm
    - Handling sensor robustness
- Documentation [Mon 20 Nov]
- Mid-route reversing [Tues 21 Nov]
