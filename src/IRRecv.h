#ifndef _IRRECV_H_
#define _IRRECV_H_

#include "IR32.h"
#include "Arduino.h"
#include "driver/rmt.h"

class IRRecv
{
  public:
    IRRecv(rmt_channel_t channel=RMT_CHANNEL_0);
    bool start(const rmt_send_timing_t* timing_group, int rx_pin);
    bool start(const rmt_send_timing_t* timing_group, gpio_num_t rx_pin);
    int8_t available();
    uint32_t read();
    void stop();
    bool active();

  private:
    bool rx_check_in_range(int duration_ticks, int target_us, int margin_us);
    bool rx_header_if(rmt_item32_t* item);
    bool rx_bit_one_if(rmt_item32_t* item);
    bool rx_bit_zero_if(rmt_item32_t* item);
    int rx_parse_items(rmt_item32_t* item, int item_num, uint16_t* addr, uint16_t* data);
    rmt_channel_t _channel;
    rmt_send_timing_t _timing;
    gpio_num_t _rx_pin;
    RingbufHandle_t _rb = NULL;
    bool _active = false;
};
#endif // _IRRECV_H_ 
