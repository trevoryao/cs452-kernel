#include <stdbool.h>

#include "uart-server.h"
#include "uassert.h"
#include "marklin-server.h"
#include "console-server.h"
#include "task.h"
#include "clock-server.h"
#include "clock.h"
#include "term-control.h"
#include "time.h"
#include "sys-clock.h"

#include "track.h"
#include "speed-data.h"
#include "speed.h"
#include "track-data.h"

#define N_TESTS 10
#define N_SENSOR 8 // per bump round

#define E7_NODE 70

#define N_SPDS 3
uint8_t SPEEDS[N_SPDS] = {7, 9, 11};

speed_data spd_data;
track_node track[TRACK_MAX];

// use sensor E7 as looping point
static void wait_sensor_activate(int marklin, int mod, int sen_bit) {
    int byte_sen_bit = (sen_bit > N_SENSOR) ? (sen_bit - N_SENSOR) : sen_bit;

    for (;;) {
        Putc(marklin, S88_RESET + mod);

        if (sen_bit > N_SENSOR) Getc(marklin); // discard

        char b = Getc(marklin);
        if (((b & (1 << (N_SENSOR - byte_sen_bit)))
            >> (N_SENSOR - byte_sen_bit)) == 0x01) { // bit matches?
            if (sen_bit <= N_SENSOR) Getc(marklin);
            break;
        }

        if (sen_bit <= N_SENSOR) Getc(marklin); // discard
    }
}

static bool check_sensor_activation(int marklin, int mod, int sen_bit) {
    int byte_sen_bit = (sen_bit > N_SENSOR) ? (sen_bit - N_SENSOR) : sen_bit;

    Putc(marklin, S88_RESET + mod);

    if (sen_bit > N_SENSOR) Getc(marklin); // discard

    char b = Getc(marklin);
    if (((b & (1 << (N_SENSOR - byte_sen_bit)))
        >> (N_SENSOR - byte_sen_bit)) == 0x01) { // bit matches?
        if (sen_bit <= N_SENSOR) Getc(marklin);
        return true;
    }

    if (sen_bit <= N_SENSOR) Getc(marklin); // discard
    return false;
}

static void sensor_discard(int marklin, int mod_no) {
    Putc(marklin, S88_RESET + mod_no);
    Getc(marklin);
    Getc(marklin);
}

static void sensor_discard_all(int marklin) {
    Putc(marklin, S88_BASE + 5);
    for (int i = 0; i < 10; ++i) Getc(marklin);
}

void constant_speed(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_loop_mm) {
    speed_t speed;
    speed_t_init(&speed);

    int num_newSpeeds = 2;
    uint8_t newSpeeds[2] = {8,10};

    uint64_t dist_mm = 0;
    track_node *node = &track[75]; // E12
    while (node != &track[47]) { // C16
        if (node->type == NODE_BRANCH) {
            dist_mm += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist_mm += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    Printf(console, "Measured Distance: %u mm\r\n", dist_mm);

    for (int i = 0; i < N_TRNS; ++i) {
        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d before sensor A7 and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        for (int k = 0; k < num_newSpeeds; ++k) {
            Printf(console, "Speed: %d\r\n", newSpeeds[k]);
            train_mod_speed(marklin, &speed, ALL_TRNS[i], newSpeeds[k]);
            sensor_discard_all(marklin);

            uint64_t t[N_TESTS];

            for (int j = 0; j < N_TESTS; ++j) {
                wait_sensor_activate(marklin, 5, 12);
                t[j] = get_curr_ticks(); // start
                Printf(console, COL_GRN "start sensor triggered (%d)\r\n" COL_RST, j);
                sensor_discard(marklin, 5);

                wait_sensor_activate(marklin, 3, 16);
                t[j] = get_curr_ticks() - t[j]; // end - start
                Printf(console, COL_GRN "end sensor triggered (%d)\r\n" COL_RST, j);
                sensor_discard(marklin, 3);
            }

            uint64_t v[N_TESTS]; // um/s

            // calculate individual velocities
            for (int j = 0; j < N_TESTS; ++j) {
            v[j] = (dist_mm * 1000000000) / t[j];
            }

            Printf(console, "All recorded time deltas and velocities:\r\n");
            time_t time_delta;
            for (int j = 0; j < N_TESTS; ++j) {
                time_from_sys_ticks(&time_delta, t[j]);
                Printf(console, "time %d: %u:%u (v: %u um/s) ", j, time_delta.sec, time_delta.tsec, v[j]);
            }
            Printf(console, "\r\n");

            // calculate average velocity
            uint64_t v_avg = 0;
            for (int j = 0; j < N_TESTS; ++j) v_avg += v[j];
            v_avg /= N_TESTS;

            Printf(console, "v_avg: %u um/s\r\n", v_avg);
        }

        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
    }
}

void constant_speed_loop(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_loop_mm) {
    speed_t speed;
    speed_t_init(&speed);

    int num_newSpeeds = 5;
    uint8_t newSpeeds[5] = {7,8,9,10,11};

    uint64_t dist_mm = 0;
    track_node *node = &track[75]; // E12
    // increment node by one to make sure we actually compute the loop
    dist_mm += node->edge[DIR_AHEAD].dist;
    node = node->edge[DIR_AHEAD].dest;
    while (node != &track[75]) { // E12 -> loop
        if (node->type == NODE_BRANCH) {
            dist_mm += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist_mm += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    Printf(console, "Measured Distance: %u mm\r\n", dist_mm);

    for (int i = 0; i < N_TRNS; ++i) {
        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d before sensor A7 and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        for (int k = 0; k < num_newSpeeds; ++k) {
            Printf(console, "Speed: %d\r\n", newSpeeds[k]);
            train_mod_speed(marklin, &speed, ALL_TRNS[i], newSpeeds[k]);
            sensor_discard_all(marklin);

            uint64_t t[N_TESTS];

            for (int j = 0; j < N_TESTS; ++j) {
                wait_sensor_activate(marklin, 5, 12);
                t[j] = get_curr_ticks(); // start
                Printf(console, COL_GRN "start sensor triggered (%d)\r\n" COL_RST, j);
                
                // Delay for long enough to not have a double delay
                Delay(clock, 50);
                sensor_discard(marklin, 5);

                // wait on E12 again
                wait_sensor_activate(marklin, 5, 12);
                t[j] = get_curr_ticks() - t[j]; // end - start
                Printf(console, COL_GRN "end sensor triggered (%d)\r\n" COL_RST, j);
                sensor_discard(marklin, 3);
            }

            uint64_t v[N_TESTS]; // um/s

            // calculate individual velocities
            for (int j = 0; j < N_TESTS; ++j) {
            v[j] = (dist_mm * 1000000000) / t[j];
            }

            Printf(console, "All recorded time deltas and velocities:\r\n");
            time_t time_delta;
            for (int j = 0; j < N_TESTS; ++j) {
                time_from_sys_ticks(&time_delta, t[j]);
                Printf(console, "time %d: %u:%u (v: %u um/s) ", j, time_delta.sec, time_delta.tsec, v[j]);
            }
            Printf(console, "\r\n");

            // calculate average velocity
            uint64_t v_avg = 0;
            for (int j = 0; j < N_TESTS; ++j) v_avg += v[j];
            v_avg /= N_TESTS;

            Printf(console, "v_avg: %u um/s\r\n", v_avg);
        }

        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
    }
}

void acceleration_speed(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_um) {
    speed_t speed;
    speed_t_init(&speed);

    const uint8_t BASE_SPD = 7;
    uint8_t START_SEN[2] = {'E' - 'A' + 1, 12};
    uint8_t SPEED_SENS[N_SPDS][2] = {{0, 0}, {'C' - 'A' + 1, 16}, {'C' - 'A' + 1, 6}};

    uint64_t dist1 = 0;

    track_node *node = &track[75]; // E13
    while (node != &track[47]) { // D13
        if (node->type == NODE_BRANCH) {
            dist1 += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist1 += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    uint64_t dist2 = dist1;
    while (node != &track[37]) { // C6
        if (node->type == NODE_BRANCH) {
            dist2 += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist2 += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    // start throws all necessary switches
    Printf(console, "Distance 1: %u mm Distance 2: %u mm\r\n", dist1, dist2);

    for (int i = 0; i < N_TRNS; ++i) {
        // train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d before sensor A7 and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        for (int k = 1; k < N_SPDS; ++k) {
            Printf(console, "Speed: %d<->%d\r\n", BASE_SPD, SPEEDS[k]);
            train_mod_speed(marklin, &speed, ALL_TRNS[i], BASE_SPD);
            sensor_discard_all(marklin);

            int64_t t[2][N_TESTS];

            for (int j = 0; j < 2 * N_TESTS; ++j) {
                wait_sensor_activate(marklin, START_SEN[0], START_SEN[1]);
                t[j % 2][j / 2] = get_curr_ticks();
                train_mod_speed(marklin, &speed, ALL_TRNS[i], (j % 2 == 0) ? SPEEDS[k] : BASE_SPD);
                Printf(console, COL_GRN "start sensor triggered (%d)\r\n" COL_RST, j);
                sensor_discard(marklin, START_SEN[0]);

                wait_sensor_activate(marklin, SPEED_SENS[k][0], SPEED_SENS[k][1]);
                t[j % 2][j / 2] = get_curr_ticks() - t[j % 2][j / 2]; // end - start
                Printf(console, COL_GRN "end sensor triggered (%d)\r\n" COL_RST, j);
                sensor_discard(marklin, SPEED_SENS[k][0]);
            }

            int64_t t_x[2][N_TESTS];
            int64_t dist = ((k - 1) % 2 == 0) ? dist1 : dist2;

            // acceleration
            int32_t v1 = get_velocity(&spd_data, ALL_TRNS[i], BASE_SPD);
            int32_t v2 = get_velocity(&spd_data, ALL_TRNS[i], SPEEDS[k]);

            int64_t a_avg = 0;
            for (int j = 0; j < N_TESTS; ++j) {
                t_x[0][j] = (2 * ((dist * 1000000000) - (v2 * t[0][j]))) / (v1 - v2);
                a_avg += (1000000LL * (v2 - v1)) / t_x[0][j];
            }

            a_avg /= N_TESTS;
            Printf(console, "Avg Acceleration (%d->%d over %d mm): %d um/s2\r\n", v1, v2, dist, a_avg);

            // deacceleration
            v1 = get_velocity(&spd_data, ALL_TRNS[i], SPEEDS[k]);
            v2 = get_velocity(&spd_data, ALL_TRNS[i], BASE_SPD);

            a_avg = 0;
            for (int j = 0; j < N_TESTS; ++j) {
                t_x[1][j] = (2 * ((dist * 1000000000) - (v2 * t[1][j]))) / (v1 - v2);
                a_avg += (1000000LL * (v2 - v1)) / t_x[1][j];
            }

            a_avg /= N_TESTS;
            Printf(console, "Avg Deacceleration (%d->%d over %d mm): %d um/s2\r\n", v1, v2, dist, a_avg);

            time_t t_t;
            time_t tx_t;

            Printf(console, "Acceleration Data:\r\n");
            for (int j = 0; j < N_TESTS; ++j) {
                time_from_sys_ticks(&t_t, t[0][j]);
                time_from_sys_ticks(&tx_t, t_x[0][j]);
                Printf(console, "(%d) measured time: %u:%u calculated time: %u:%u\t\t", j, t_t.sec, t_t.tsec, tx_t.sec, tx_t.tsec);
            }
            Printf(console, "\r\n");

            Printf(console, "Deacceleration Data:\r\n");
            for (int j = 0; j < N_TESTS; ++j) {
                time_from_sys_ticks(&t_t, t[1][j]);
                time_from_sys_ticks(&tx_t, t_x[1][j]);
                Printf(console, "(%d) measured time: %u:%u calculated time: %u:%u\t\t", j, t_t.sec, t_t.tsec, tx_t.sec, tx_t.tsec);
            }
            Printf(console, "\r\n");
        }

        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
    }
}

void acceleration_speed_adaptive(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_um, uint8_t base_speed, uint8_t goal_speed) {
    speed_t speed;
    speed_t_init(&speed);

    const uint8_t BASE_SPD = base_speed;
    uint8_t START_SEN[2] = {'E' - 'A' + 1, 12};
    uint8_t SPEED_SENS[2] = {'C' - 'A' + 1, 6};

    int64_t dist = 0;

    // Compute the distance of measurement
    track_node *node = &track[75]; // E12
    while (node != &track[37]) { // C6
        if (node->type == NODE_BRANCH) {
            dist += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    // start throws all necessary switches
    Printf(console, "Distance 1: %u mm\r\n", dist);

    for (int i = 0; i < N_TRNS; ++i) {
        // train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d before sensor A7 and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        Printf(console, "Speed: %d<->%d\r\n", BASE_SPD, goal_speed);
        train_mod_speed(marklin, &speed, ALL_TRNS[i], BASE_SPD);
        sensor_discard_all(marklin);

        // run it in a loop two times 1 loop acceleration, second deacceleration
        wait_sensor_activate(marklin, START_SEN[0], START_SEN[1]);
        Printf(console, "running train in a loop once\r\n");
        train_mod_speed(marklin, &speed, ALL_TRNS[i], goal_speed);
        sensor_discard_all(marklin);

        wait_sensor_activate(marklin, SPEED_SENS[0], SPEED_SENS[1]);
        sensor_discard_all(marklin);

        // second loop back to base speed
        wait_sensor_activate(marklin, START_SEN[0], START_SEN[1]);
        train_mod_speed(marklin, &speed, ALL_TRNS[i], BASE_SPD);
        sensor_discard_all(marklin);

        wait_sensor_activate(marklin, SPEED_SENS[0], SPEED_SENS[1]);
        sensor_discard_all(marklin);


        Printf(console, "reached end sensor - discarding sensor data and starting measurements\r\n");


        int64_t t[2][N_TESTS];

        for (int j = 0; j < 2 * N_TESTS; ++j) {
            // WTF?
            wait_sensor_activate(marklin, START_SEN[0], START_SEN[1]);
            t[j % 2][j / 2] = get_curr_ticks();
            train_mod_speed(marklin, &speed, ALL_TRNS[i], (j % 2 == 0) ? goal_speed : BASE_SPD);
            Printf(console, COL_GRN "start sensor triggered (%d)\r\n" COL_RST, j);
            sensor_discard(marklin, START_SEN[0]);

            wait_sensor_activate(marklin, SPEED_SENS[0], SPEED_SENS[1]);
            t[j % 2][j / 2] = get_curr_ticks() - t[j % 2][j / 2]; // end - start
            Printf(console, COL_GRN "end sensor triggered (%d)\r\n" COL_RST, j);
            sensor_discard(marklin, SPEED_SENS[0]);
        }

        int64_t t_x[2][N_TESTS];

        // acceleration
        int64_t v1 = get_velocity(&spd_data, ALL_TRNS[i], BASE_SPD);
        int64_t v2 = get_velocity(&spd_data, ALL_TRNS[i], goal_speed);
        Printf(console, "Got velocities v1 %d and v2 %d\r\n", v1, v2);


        int64_t a_avg = 0;
        for (int j = 0; j < N_TESTS; ++j) {
            t_x[0][j] = (2 * ((dist * 1000000000) - (v2 * t[0][j]))) / (v1 - v2);
            a_avg += (1000000LL * (v2 - v1)) / t_x[0][j];

            Printf(console, "Measured time t: %d, t_x: %d, a_avg %d, dist %d\r\n", t[0][j], t_x[0][j], a_avg, dist);
        }

        a_avg /= N_TESTS;
        Printf(console, "Avg Acceleration (%d->%d over %d mm): %d um/s2\r\n", v1, v2, dist, a_avg);

        // deacceleration
        v1 = get_velocity(&spd_data, ALL_TRNS[i], goal_speed);
        v2 = get_velocity(&spd_data, ALL_TRNS[i], BASE_SPD);

        a_avg = 0;
        for (int j = 0; j < N_TESTS; ++j) {
            t_x[1][j] = (2 * ((dist * 1000000000) - (v2 * t[1][j]))) / (v1 - v2);
            a_avg += (1000000LL * (v2 - v1)) / t_x[1][j];
        }

        a_avg /= N_TESTS;
        Printf(console, "Avg Deacceleration (%d->%d over %d mm): %d um/s2\r\n", v1, v2, dist, a_avg);

        time_t t_t;
        time_t tx_t;

        Printf(console, "Acceleration Data:\r\n");
        for (int j = 0; j < N_TESTS; ++j) {
            time_from_sys_ticks(&t_t, t[0][j]);
            time_from_sys_ticks(&tx_t, t_x[0][j]);
            Printf(console, "(%d) measured time: %u:%u calculated time: %u:%u\t\t", j, t_t.sec, t_t.tsec, tx_t.sec, tx_t.tsec);
        }
        Printf(console, "\r\n");

        Printf(console, "Deacceleration Data:\r\n");
        for (int j = 0; j < N_TESTS; ++j) {
            time_from_sys_ticks(&t_t, t[1][j]);
            time_from_sys_ticks(&tx_t, t_x[1][j]);
            Printf(console, "(%d) measured time: %u:%u calculated time: %u:%u\t\t", j, t_t.sec, t_t.tsec, tx_t.sec, tx_t.tsec);
        }
        Printf(console, "\r\n");

        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
    }
}

void stopping_distance(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_loop_mm) {
    speed_t speed;
    speed_t_init(&speed);

    int64_t dist_mm = 0;
    track_node *node = &track[75]; // E12
    while (node != &track[37]) { // C6
        if (node->type == NODE_BRANCH) {
            dist_mm += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist_mm += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    Printf(console, "Measured Distance (E12 -> C6): %u mm\r\n", dist_mm);

    for (int i = 1; i < N_TRNS; ++i) {
        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        sensor_discard_all(marklin);

        for (int j = 0; j < N_SPDS; ++j) {
            Printf(console, "Testing Speed %d\r\n", SPEEDS[j]);
            int32_t v = get_velocity(&spd_data, ALL_TRNS[i], SPEEDS[j]);

            int delay = 500; // 5000ms initial delay
            int change = (delay >> 1); // div 2
            char c;
            for (;;) {
                Printf(console, "Testing %d ms delay\r\n", delay * TICK_MS);
                train_mod_speed(marklin, &speed, ALL_TRNS[i], SPEEDS[j]);

                wait_sensor_activate(marklin, 5, 12);
                Printf(console, COL_GRN "Registered!\r\n" COL_RST);
                sensor_discard(marklin, 5);

                Delay(clock, delay);

                train_mod_speed(marklin, &speed, ALL_TRNS[i], SPD_STP);

                Delay(clock, 500); // let it stop

                // NOTE: may still be overshot
                if (check_sensor_activation(marklin, 3, 6))
                    Printf(console, COL_GRN "Registered end sensor!\r\n" COL_RST);

                // delay is tx
                // stopping distance is dist - v * tx
                int32_t stopping_dist = ((dist_mm * 1000000) - (v * delay * TICK_MS)) / 1000;
                Printf(console, "Calculated stopping distance: %d um (%d mm)\r\n", stopping_dist, stopping_dist / 1000);

                for (;;) {
                    Printf(console, "Enter Test result (*l*ess delay / *m*ore delay / *d*one): ");
                    c = Getc(console);

                    switch (c) {
                        case 'l': delay -= change; break;
                        case 'm': delay += change; break;
                        case 'd': goto Found;
                        default: Printf(console, "\r\nTry Again! Enter one of l, m, d\r\n"); continue;
                    }

                    change = (change >> 1); // div 2

                    break;
                }

                Printf(console, "%c\r\n", c);
            }
Found:;
        }
    }
}

void manual_testing(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_loop_mm, uint32_t delay, uint16_t trainNo) {
    speed_t speed;
    speed_t_init(&speed);

    int64_t dist_mm = 0;
    track_node *node = &track[75]; // E12
    while (node != &track[37]) { // C6
        if (node->type == NODE_BRANCH) {
            dist_mm += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist_mm += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    Printf(console, "Measured Distance (E12 -> C6): %u mm\r\n", dist_mm);

    train_mod_speed(marklin, &speed, trainNo, 0);

    Printf(console, "Place Train %d and press any key: ", trainNo);
    Getc(console);
    Puts(console, "\r\n");

    sensor_discard_all(marklin);

    int32_t v = get_velocity(&spd_data, trainNo, 7);
    Printf(console, "Testing %d ms delay\r\n", delay * TICK_MS);

    for (;;) {
        train_mod_speed(marklin, &speed, trainNo, 7);
        wait_sensor_activate(marklin, 5, 12);
        Printf(console, COL_GRN "Registered!\r\n" COL_RST);
        sensor_discard(marklin, 5);

        Delay(clock, delay);

        train_mod_speed(marklin, &speed, trainNo, 0);

        Delay(clock, 500); // let it stop

        // NOTE: may still be overshot
        if (check_sensor_activation(marklin, 3, 6))
            Printf(console, COL_GRN "Registered end sensor!\r\n" COL_RST);

        // delay is tx
        // stopping distance is dist - v * tx

    }
}

void stop_train_at_sensor(uint16_t clock, uint16_t console, uint16_t marklin, speed_t *speed, uint16_t trainNo, int64_t dist_um) {
    sensor_discard_all(marklin);
    int32_t v = get_velocity(&spd_data, trainNo, 7);

    // get breaking distance
    int32_t breaking_distance = get_stopping_distance(&spd_data, trainNo, 7);

    // Calculate expected delay
    int64_t scaling = 1000; // scale by 100 for proper clock ticks
    int64_t delay_nominator = (dist_um - breaking_distance) * scaling;

    int64_t delay = (delay_nominator / v) / TICK_MS;
    Printf(console, "Breaking train with delay %d ms \r\n", delay * TICK_MS);

    // actual train break;
    train_mod_speed(marklin, speed, trainNo, 7);
    wait_sensor_activate(marklin, 4, 9);
    Printf(console, COL_GRN "Registered!\r\n" COL_RST);
    Delay(clock, delay);

    train_mod_speed(marklin, speed, trainNo, 0);
    sensor_discard(marklin, 4);
}

void acceleration_from_zero(uint16_t clock, uint16_t console, uint16_t marklin, uint64_t dist_um, uint8_t target_spd) {
    speed_t speed;
    speed_t_init(&speed);

    const uint8_t TARGET_SPD = target_spd;

    int64_t dist = 0;
    track_node *node = &track[58]; // D11
    while (node != &track[37]) { // C6
        if (node->type == NODE_BRANCH) {
            dist += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }

    // Calculate the distance
    int64_t dist_stop = 0;
    node = &track[56]; // D9
    while (node != &track[58]) { // D11
        if (node->type == NODE_BRANCH) {
            dist_stop += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            dist_stop += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    }
    dist_stop = dist_stop * 1000;

    // start throws all necessary switches
    Printf(console, "Distance: %u mm\r\n", dist);

    for (int i = 0; i < N_TRNS; ++i) {
        // train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
        Printf(console, "Place Train %d before sensor A7 and press any key: ", ALL_TRNS[i]);
        Getc(console);
        Puts(console, "\r\n");

        Printf(console, "Speed: 0 -> %d\r\n", target_spd);

        int64_t t[N_TESTS];

        for (int j = 0; j < N_TESTS; ++j) {
            stop_train_at_sensor(clock, console, marklin, &speed, ALL_TRNS[i], dist_stop);

            Printf(console, "Adjust Train %d at sensor D11 and press any key: ", ALL_TRNS[i]);
            Getc(console);
            Puts(console, "\r\n");

            train_mod_speed(marklin, &speed, ALL_TRNS[i], TARGET_SPD);
            t[j] = get_curr_ticks();

            wait_sensor_activate(marklin, 3, 6); // C6
            t[j] = get_curr_ticks() - t[j];
            Printf(console, COL_GRN "end sensor triggered (%d)\r\n" COL_RST, j);
            sensor_discard(marklin, 3);
        }

        int t_x[N_TESTS];

        int32_t v1 = 0;
        int32_t v2 = get_velocity(&spd_data, ALL_TRNS[i], TARGET_SPD);

        int64_t a_avg = 0;
        for (int j = 0; j < N_TESTS; ++j) {
            t_x[j] = (2 * ((dist * 1000000000) - (v2 * t[j]))) / (v1 - v2);
            a_avg += (1000000LL * (v2 - v1)) / t_x[j];
        }
        a_avg /= N_TESTS;
        Printf(console, "Avg Acceleration (%d->%d over %d mm): %d um/s2\r\n", v1, v2, dist, a_avg);

        time_t t_t;
        time_t tx_t;

        Printf(console, "Acceleration Data:\r\n");
        for (int j = 0; j < N_TESTS; ++j) {
            time_from_sys_ticks(&t_t, t[j]);
            time_from_sys_ticks(&tx_t, t_x[j]);
            Printf(console, "(%d) measured time: %u:%u calculated time: %u:%u\t\t", j, t_t.sec, t_t.tsec, tx_t.sec, tx_t.tsec);
        }
        Printf(console, "\r\n");

        train_mod_speed(marklin, &speed, ALL_TRNS[i], 0);
    }
}

void short_moves(int clock,  int console, int marklin) { 
    speed_t speed;
    speed_t_init(&speed);

    uint8_t trainNo = 24;
    uint8_t TARGET_SPD = 7;
    uint8_t amount_delays = 15;
    uint32_t TIME_DELAYS[18] = {575, 550, 525, 500, 150, 175, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475};
    int i = 0;
    while (i < 17) {
        Printf(console, "Place Train %d at start - running with time %d\r\n", trainNo, TIME_DELAYS[i]);
        Getc(console);
        Puts(console, "\r\n");

        int time = Time(clock);
        train_mod_speed(marklin, &speed, trainNo, TARGET_SPD);

        DelayUntil(clock, time + TIME_DELAYS[i]);

        train_mod_speed(marklin, &speed, trainNo, 0);
        Printf(console, "repeat or continue?\r\n", trainNo, TIME_DELAYS[i]);
        char c = Getc(console);
        if (c != 'r') {
            i = i+1;
        }
    } 
    
}

void short_move_test(int clock,  int console, int marklin) {
    speed_t speed;
    speed_t_init(&speed);

    int trainNo = 77;
    int distances[12] = {200000, 250000, 300000, 350000, 400000, 450000, 500000, 550000, 600000, 650000, 700000, 750000};
    for (int i = 0; i < 12; i++) {
        int delay = get_short_move_delay(&spd_data, trainNo, distances[i]);
        Printf(console, "Testing train %d with distance %d and delay %d\r\n", trainNo, distances[i], get_short_move_delay(&spd_data, 77, distances[i]));
        Getc(console);

        train_mod_speed(marklin, &speed, trainNo, 7);

        Delay(clock, delay);

        train_mod_speed(marklin, &speed, trainNo, 0);

    }
}

void user_main(void) {
    // start up clock, uart servers
    uint16_t clock = Create(P_SERVER_HI, clockserver_main);
    uint16_t console = Create(P_SERVER_LO, console_server_main);
    uint16_t marklin = Create(P_SERVER_HI, marklin_server_main);

    // reset track to get our loop
    speed_data_init(&spd_data);
    init_track_b(track);

    uint64_t dist_um = 0;
    track_node *node = &track[E7_NODE];

    do {
        if (node->type == NODE_BRANCH) {
            // make sure track set right
            switch_throw(marklin, node->num, STRT);
            dist_um += node->edge[DIR_STRAIGHT].dist;
            node = node->edge[DIR_STRAIGHT].dest;
        } else {
            if (node->type == NODE_MERGE) {
                if (node->edge[DIR_AHEAD].dest->edge[DIR_STRAIGHT].src == node) {
                    switch_throw(marklin, node->num, STRT);
                } else {
                    switch_throw(marklin, node->num, CRV);
                }
            }

            dist_um += node->edge[DIR_AHEAD].dist;
            node = node->edge[DIR_AHEAD].dest;
        }
    } while (node != &track[E7_NODE]);

    Printf(console, "Track Loop Distance: %u um\r\n", dist_um * 1000);

    // constant_speed(clock, console, marklin, dist_um);
    // acceleration_speed(clock, console, marklin, dist_um);
    // stopping_distance(clock, console, marklin, dist_um);
    // manual_testing(clock, console, marklin, dist_um, 520, 24);
    // stop_train_at_sensor(clock, console, marklin, 58);
    // acceleration_from_zero(clock, console, marklin, dist_um);
    //acceleration_speed_adaptive(clock, console, marklin, dist_um, 9, 11);

    //short_move_test(clock, console, marklin);
    //acceleration_from_zero(clock, console, marklin, dist_um, 9);

    constant_speed(clock, console, marklin, dist_um);

    WaitOutputEmpty(marklin);
}
