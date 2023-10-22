#include "k4/sensors.h"

#include "uart-server.h"

#include "k4/controller-consts.h"
#include "k4/monitor.h"

#define N_SENSOR 8 // per dump round

void sen_data_init(struct sen_data *d) {
    d->mod_num = 0;
    d->mod_round = 0;
}

void sen_start_dump(uint16_t tid) {
    Putc(tid, S88_BASE + N_S88);
}

int rcv_sen_dump(struct sen_data *d, char b, uint16_t tid, deque *recent_sens) {
    if (b != 0x00) { // save time for common case
        for (uint8_t sen_bit = 1; sen_bit <= N_SENSOR; ++sen_bit) {
            if (((b & (1 << (N_SENSOR - sen_bit))) >> (N_SENSOR - sen_bit)) == 0x01) { // bit matches?
                int16_t sensor = sen_bit + (d->mod_round * N_SENSOR);

                update_triggered_sensor(tid, recent_sens, d->mod_num, sensor);
            }
        }
    }

    if (d->mod_round == 1) {
        if (d->mod_num == N_S88 - 1) { // done
            sen_data_init(d);
            return 1; // done
        } else {
            d->mod_round = 0;
            ++d->mod_num;
        }
    } else {
        ++d->mod_round;
    }

    return 0;
}
