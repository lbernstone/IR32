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

bool IRRecv::rx_check_in_range(int duration_ticks, int target_us, int margin_us)
{
    if(( RMT_ITEM_DURATION(duration_ticks) < (target_us + margin_us))
        && ( RMT_ITEM_DURATION(duration_ticks) > (target_us - margin_us))) {
        return true;
    } else {
        return false;
    }
}

bool IRRecv::rx_header_if(rmt_item32_t* item)
{
    if((item->level0 != _timing.invert && item->level1 == _timing.invert)
        && rx_check_in_range(item->duration0, _timing.header_mark_us, _timing.bit_margin)
        && rx_check_in_range(item->duration1, _timing.zero_mark_us, _timing.bit_margin)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_one_if(rmt_item32_t* item)
{
    if((item->level0 != _timing.invert && item->level1 == _timing.invert)
        && rx_check_in_range(item->duration0, _timing.one_mark_us, _timing.bit_margin)
        && rx_check_in_range(item->duration1, _timing.one_space_us, _timing.bit_margin)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_zero_if(rmt_item32_t* item)
{
    if((item->level0 != _timing.invert && item->level1 == _timing.invert)
        && rx_check_in_range(item->duration0, _timing.zero_mark_us, _timing.bit_margin)
        && rx_check_in_range(item->duration1, _timing.zero_space_us, _timing.bit_margin)) {
        return true;
    }
    return false;
}

int IRRecv::rx_parse_items(rmt_item32_t* item, int item_num, uint16_t* addr, uint16_t* data)
{
    int w_len = item_num;
    if(w_len < _timing.bit_length + 2) {
        log_e("Item length was only %d bit", w_len);
        return -1;
    }
    int i = 0, j = 0;
    if(!rx_header_if(item++)) {
        return -1;
    }
    uint16_t addr_t = 0;
    for(j = 0; j < 16; j++) {
        if(rx_bit_one_if(item)) {
            addr_t |= (1 << j);
        } else if(rx_bit_zero_if(item)) {
            addr_t |= (0 << j);
        } else {
            return -1;
        }
        item++;
        i++;
    }
    uint16_t data_t = 0;
    for(j = 0; j < 16; j++) {
        if(rx_bit_one_if(item)) {
            data_t |= (1 << j);
        } else if(rx_bit_zero_if(item)) {
            data_t |= (0 << j);
        } else {
            return -1;
        }
        item++;
        i++;
    }
    *addr = addr_t;
    *data = data_t;
    return i;
}

uint32_t IRRecv::read()
{
    if (!available()) return NULL;
    size_t rx_size = 0;
    rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(_rb, &rx_size, RMT_RX_BUF_WAIT);
    if (!item) return NULL;
    uint16_t rmt_addr;
    uint16_t rmt_cmd;
    int offset = 0;
    while(1) {
        //parse data value from ringbuffer.
        int res = rx_parse_items(item + offset, rx_size / 4 - offset, &rmt_addr, &rmt_cmd);
        if(res > 0) {
            offset += res + 1;
            log_i("RMT RCV --- addr: 0x%04x cmd: 0x%04x", rmt_addr, rmt_cmd);
        } else {
            break;
        }
    }
    //after parsing the data, clear space in the ringbuffer.
    vRingbufferReturnItem(_rb, (void*) item);
    log_v("Available after return: %d",available());
    return rmt_cmd;
}    

void IRRecv::stop()
{
    rmt_driver_uninstall(_channel);
    _rx_pin = GPIO_NUM_MAX;
    _timing = {};
    _active = false;
    vRingbufferDelete(_rb);
}

bool IRRecv::active() {return _active;}    
