#ifndef _IRSEND_H_
#define _IRSEND_H_

#include "IR32.h"
#include "Arduino.h"
#include "driver/rmt.h"
#include <stdexcept>      // std::out_of_range

class IRSend
{
  public:
    IRSend(rmt_channel_t channel=RMT_CHANNEL_1);
    bool startRMT(uint8_t timing);
    bool start(int tx_pin, const char* timingGroup = "NEC");
    bool start(gpio_num_t tx_pin, const char* timingGroup);
    bool start(int tx_pin, String timingGroup);
    bool start(gpio_num_t tx_pin, String timingGroup);
    bool send(uint32_t code);
    bool send(uint32_t code, uint8_t timing);
    bool send(uint32_t code, const char* timingGroup);
    bool send(std::string code);
    bool send(std::string code, uint8_t timing);
    void stop();
    bool active();

  private:
    void rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us);
    void rmt_fill_item_header(rmt_item32_t* item);
    void rmt_fill_item_bit_one(rmt_item32_t* item);
    void rmt_fill_item_bit_zero(rmt_item32_t* item);
    void rmt_fill_item_end(rmt_item32_t* item);
    void rmt_build_item(rmt_item32_t* item, uint32_t cmd_data);
    void nec_build_item(rmt_item32_t* item, uint32_t cmd_data);

    rmt_channel_t _channel;
    uint8_t _timing;
    gpio_num_t _tx_pin;
    bool _active = false;
};
#endif // _IRSEND_H_
