#ifndef _IRSEND_H_
#define _IRSEND_H_

#include "IR32.h"
#include "Arduino.h"
#include "driver/rmt.h"

class IRSend
{
  public:
    IRSend(rmt_channel_t channel=RMT_CHANNEL_1);
    bool start(const rmt_send_timing_t* timing_group, int tx_pin);
    bool start(const rmt_send_timing_t* timing_group, gpio_num_t tx_pin);
    bool send(uint32_t code);
    void stop();
    bool active();

  private:
    inline void rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us);
    void rmt_fill_item_header(rmt_item32_t* item);
    void rmt_fill_item_bit_one(rmt_item32_t* item);
    void rmt_fill_item_bit_zero(rmt_item32_t* item);
    void rmt_fill_item_end(rmt_item32_t* item);
    void rmt_build_item(rmt_item32_t* item, uint32_t cmd_data);
    void nec_build_item(rmt_item32_t* item, uint32_t cmd_data);

    rmt_channel_t _channel;
    rmt_send_timing_t _timing;
    gpio_num_t _tx_pin;
    bool _active = false;
};
#endif // _IRSEND_H_
