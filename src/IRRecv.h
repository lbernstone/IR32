#ifndef _IRRECV_H_
#define _IRRECV_H_

#include "IR32.h"
#include "Arduino.h"
#include "driver/rmt.h"
#include <algorithm>

class IRRecv
{
  public:
    IRRecv(rmt_channel_t channel=RMT_CHANNEL_0);
    bool start(const rmt_send_timing_t* timing_group, int rx_pin);
    bool start(const rmt_send_timing_t* timing_group, gpio_num_t rx_pin);
    int8_t available();
    uint32_t read();
    void setMargin(uint8_t margin_pct);
    bool inPrefVector(uint8_t element);
    int setPreferred(char* timing_group);
    int setPreferred(String timing_group);
    void stop();
    bool active();

  private:
    bool rx_check_in_range(int duration_ticks, int target_us);
    bool rx_header_if(rmt_item32_t* item);
    bool rx_bit_one_if(rmt_item32_t* item);
    bool rx_bit_zero_if(rmt_item32_t* item);
    uint32_t rx_parse_items(rmt_item32_t* item, int item_num);
    rmt_channel_t _channel;
    rmt_send_timing_t _timing;
    gpio_num_t _rx_pin;
    uint8_t _margin_pct = 15;
    std::vector<uint8_t> _preferred;
    RingbufHandle_t _rb = NULL;
    bool _active = false;
};
#endif // _IRRECV_H_ 
