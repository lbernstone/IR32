#ifndef _IR_RMT_SENDDATA_H_
#define _IR_RMT_SENDDATA_H_

#include "Arduino.h"

typedef struct {
    const char* tag;
    uint16_t carrier_freq_khz;
    uint8_t duty_cycle;
    uint8_t bit_length;
    bool invert;
    uint16_t header_mark_us;
    uint16_t header_space_us;
    uint16_t one_mark_us;
    uint16_t one_space_us;
    uint16_t zero_mark_us;
    uint16_t zero_space_us;
    uint16_t end_wait_us;
} rmt_send_timing_t;

    const rmt_send_timing_t NEC_timing = {"NEC", 38000, 33, 32, 0, 9000, 4500, 560, 1690, 560, 560, 560};
    const rmt_send_timing_t LG_timing = {"LG", 38000, 33, 28, 0, 8500, 4250, 560, 1600, 560, 560, 800};
    const rmt_send_timing_t LG32_timing = {"LG32", 38000, 33, 32, 0, 4500, 4500, 500, 1750, 500, 560, 8950};
    const rmt_send_timing_t SAMSUNG_timing = {"samsung", 38000, 33, 32, 0, 4500, 4450, 560, 1600, 560, 560, 8950};
    const rmt_send_timing_t timing_groups[] = {
      {"NEC", 38000, 33, 32, 0, 9000, 4500, 560, 1690, 560, 560, 560},
      {"LG", 38000, 33, 28, 0, 8500, 4250, 560, 1600, 560, 560, 800},
      {"LG32", 38000, 33, 32, 0, 4500, 4500, 500, 1750, 500, 560, 8950},
      {"samsung", 38000, 33, 32, 0, 4500, 4450, 560, 1600, 560, 560, 8950}
    };
#endif // _IR_RMT_SENDDATA_H_
