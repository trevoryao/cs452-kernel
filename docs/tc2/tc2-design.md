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
- Positioning & Locking [Fri 17 Nov]
    - Finish sensor timeout testing
    - New Segment definition
    - Add deadlock detection to locking server
    - Changes to routing algorithm:
        - continuous route re-computation
        - lockstep locking
        - speed changes in routing algorithm
        - reversing mid-route
        - detect and make short move (expected sensor?)
- Others [Sun 19 Nov]
    - Sensor robustness
        - See notes
    - Auto-initialisation
        - Move train until we hit a sensor, get next sensor w/ timeout if not on track
- Documentation [Mon 20 Nov]
