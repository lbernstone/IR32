#include "IRRecv.h"

#define RMT_RX_BUF_SIZE 1000
#define RMT_RX_BUF_WAIT 10
#define RMT_ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */
#define RMT_FILTER_THRESH 100 // ticks
#define RMT_IDLE_TIMEOUT 8000 // ticks

IRRecv::IRRecv(rmt_channel_t channel)
{
    if (channel >= RMT_CHANNEL_MAX) {
        log_e("Invalid RMT channel: %d", channel);
    } else {
        _channel = channel;
    }
    _timing = {};
    _rx_pin = GPIO_NUM_MAX;
}

bool IRRecv::start(const rmt_send_timing_t* timing_group, gpio_num_t rx_pin)
{
    rmt_config_t rmt_rx;
    rmt_rx.channel = _channel;
    rmt_rx.gpio_num = rx_pin;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = true;
    rmt_rx.rx_config.filter_ticks_thresh = RMT_FILTER_THRESH;
    rmt_rx.rx_config.idle_threshold = RMT_IDLE_TIMEOUT;

    if (rmt_config(&rmt_rx) != ESP_OK) return false;
    if (rmt_driver_install(_channel, RMT_RX_BUF_SIZE, 0) != ESP_OK) return false;
    _rb = NULL;
    rmt_get_ringbuf_handle(_channel, &_rb);
    rmt_rx_start(_channel, 1);
    _timing = *timing_group;
    _rx_pin = rx_pin;
    _active = true;
}

bool IRRecv::start(const rmt_send_timing_t* timing_group, int rx_pin)
{
    return start(timing_group, (gpio_num_t) rx_pin);
}

int8_t IRRecv::available()
{
   if (!_active) return -1;
   UBaseType_t waiting;
   vRingbufferGetInfo(_rb, NULL, NULL, NULL, &waiting);
   return waiting;
} 

bool IRRecv::rx_check_in_range(int duration_ticks, int target_us)
{
    uint16_t margin_us = _margin_pct * target_us / 100;
    if(( RMT_ITEM_DURATION(duration_ticks) < (target_us + margin_us))
        && ( RMT_ITEM_DURATION(duration_ticks) > (target_us - margin_us))) {
        return true;
    } else {
        return false;
    }
}

bool IRRecv::rx_header_if(rmt_item32_t* item)
{
    if(rx_check_in_range(item->duration0, _timing.header_mark_us)
        && rx_check_in_range(item->duration1, _timing.header_space_us)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_one_if(rmt_item32_t* item)
{
    if( rx_check_in_range(item->duration0, _timing.one_mark_us)
        && rx_check_in_range(item->duration1, _timing.one_space_us)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_zero_if(rmt_item32_t* item)
{
    if( rx_check_in_range(item->duration0, _timing.zero_mark_us)
        && rx_check_in_range(item->duration1, _timing.zero_space_us)) {
        return true;
    }
    return false;
}

uint32_t IRRecv::rx_parse_items(rmt_item32_t* item, int item_num)
{
    int w_len = item_num;
    if(w_len < _timing.bit_length + 2) {
        log_e("Item length was only %d bit", w_len);
        return -1;
    }
    if(!rx_header_if(item++)) {
        return -1;
    }
    uint32_t data = 0;
    for(uint8_t j = 0; j < _timing.bit_length; j++) {
        if(rx_bit_one_if(item)) {
            data <<= 1;
            data += 1;
        } else if(rx_bit_zero_if(item)) {
            data <<= 1;
        } else {
            return -1;
        }
        item++;
    }
    return data;
}

void dump_item(rmt_item32_t* item, size_t sz)
{
  for (int x=0; x<sz; x++) {
    log_v("Count: %d  duration0: %d  duration1: %d\n", x,item[x].duration0,item[x].duration1);
    if(item[x].duration1==0 || item[x].duration0 == 0 || item[x].duration1 > 0x7f00 || item[x].duration0 > 0x7f00) break;
  }
}
 
uint32_t IRRecv::read()
{
    if (!available()) return NULL;
    size_t rx_size = 0;
    rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(_rb, &rx_size, RMT_RX_BUF_WAIT);
    if (!item) return NULL;
    dump_item(item,rx_size); 
    uint32_t rx_data;
    int offset = 0;
    rx_data = rx_parse_items(item + offset, rx_size / 4 - offset);
    //after parsing the data, clear space in the ringbuffer.
    vRingbufferReturnItem(_rb, (void*) item);
    return rx_data;
}    

void IRRecv::setMargin(uint8_t margin_pct) {_margin_pct = margin_pct;}

void IRRecv::stop()
{
    rmt_driver_uninstall(_channel);
    _rx_pin = GPIO_NUM_MAX;
    _timing = {};
    _active = false;
    vRingbufferDelete(_rb);
}

bool IRRecv::active() {return _active;}    
