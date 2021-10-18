#include "IRRecv.h"
#include <algorithm>

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

bool IRRecv::start(gpio_num_t rx_pin)
{
    rmt_config_t rmt_rx = RMT_DEFAULT_CONFIG_RX(rx_pin, _channel);
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
    _rx_pin = rx_pin;
    _active = true;
    return true;
}
bool IRRecv::start(int rx_pin)
    {return start((gpio_num_t) rx_pin);}

int8_t IRRecv::available()
{
   if (!_active) return -1;
   UBaseType_t waiting;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4,0,0) // versions 4+ added an extra arg to vRingbufferGetInfo
   vRingbufferGetInfo(_rb, NULL, NULL, NULL, NULL, &waiting);
#else
   vRingbufferGetInfo(_rb, NULL, NULL, NULL, &waiting);
#endif
   return waiting;
} 

bool IRRecv::rx_check_in_range(int duration_ticks, int target_us)
{
    if(( RMT_ITEM_DURATION(duration_ticks) < (target_us + _margin_us))
        && ( RMT_ITEM_DURATION(duration_ticks) > (target_us - _margin_us))) {
        return true;
    } else {
        return false;
    }
}

bool IRRecv::rx_header_if(rmt_item32_t* item, uint8_t timing)
{
    if(rx_check_in_range(item->duration0, timing_groups[timing].header_mark_us)
        && rx_check_in_range(item->duration1, timing_groups[timing].header_space_us)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_one_if(rmt_item32_t* item, uint8_t timing)
{
    if( rx_check_in_range(item->duration0, timing_groups[timing].one_mark_us)
        && rx_check_in_range(item->duration1, timing_groups[timing].one_space_us)) {
        return true;
    }
    return false;
}

bool IRRecv::rx_bit_zero_if(rmt_item32_t* item, uint8_t timing)
{
    if( rx_check_in_range(item->duration0, timing_groups[timing].zero_mark_us)
        && rx_check_in_range(item->duration1, timing_groups[timing].zero_space_us)) {
        return true;
    }
    return false;
}

uint32_t IRRecv::rx_parse_items(rmt_item32_t* item, int item_num, uint8_t timing)
{
    int w_len = item_num;
    if(w_len < timing_groups[timing].bit_length + 2) {
        log_v("Item length was only %d bit", w_len);
        return 0;
    }
    if(!rx_header_if(item++, timing)) {
        return 0;
    }
    uint32_t data = 0;
    for(uint8_t j = 0; j < timing_groups[timing].bit_length; j++) {
        if(rx_bit_one_if(item, timing)) {
            data <<= 1;
            data += 1;
        } else if(rx_bit_zero_if(item, timing)) {
            data <<= 1;
        } else {
            return 0;
        }
        item++;
    }
    return data;
}

void dump_item(rmt_item32_t* item, size_t sz)
{
  for (int x=0; x<sz; x++) {
    // print item times in microseconds so its easy to use this to build a new timing entry.
    log_i("Count: %d  duration0: %dus  duration1: %dus", x,RMT_ITEM_DURATION(item[x].duration0),RMT_ITEM_DURATION(item[x].duration1));
    if(item[x].duration1==0 || item[x].duration0 == 0 || item[x].duration1 > 0x7f00 || item[x].duration0 > 0x7f00) break;
  }
}

uint32_t IRRecv::read(char* &timingGroup, bool preferredOnly)
{
    if (!available()) return 0;
         
    size_t rx_size = 0;
    rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(_rb, &rx_size, RMT_RX_BUF_WAIT);
    if (!item) return 0;
    uint32_t rx_data;
    uint8_t found_timing = 0;
    for (uint8_t timing : _preferred) {
        rx_data = rx_parse_items(item, rx_size / 4, timing);
        if (rx_data) {
            found_timing = timing;
            break;
        }
    }
    // if we did not parse the item from the prefered list then
    // check the non-prefered items as well, but only if
    // preferredOnly is not set.
    if (!rx_data && !preferredOnly) {
        uint8_t groupCount = sizeof(timing_groups)/sizeof(timing_groups[0]);
        for (uint8_t timing = 0; timing < groupCount; timing++) {
            if (!inPrefVector(timing)) {
                rx_data = rx_parse_items(item, rx_size / 4, timing);
            }
            if (rx_data) {
                found_timing = timing;
                break;
            }
        }
    }
    if (found_timing) {
        timingGroup = (char*) timing_groups[found_timing].tag;
    } else {
        log_w("read() item with length %u not parsed!", rx_size / 4);
        if (_dump_unknown) {
            dump_item(item,rx_size); 
        }
    }
    //after parsing the data, clear space in the ringbuffer.
    vRingbufferReturnItem(_rb, (void*) item);
    return rx_data;
}    

void IRRecv::setMargin(uint16_t margin_us) {_margin_us = margin_us;}

void IRRecv::setDumpUnknown(bool dump) {_dump_unknown = dump;}

uint8_t timingGroupElement(const char* tag)
{
   uint8_t counter = 0;
   for (rmt_timing_t timing : timing_groups) {
       if(timing.tag == tag) return counter;
       counter++;
   }
   return 0;
}

bool IRRecv::inPrefVector(uint8_t element)
{
    for (int x : _preferred) if (x == element) return true;
    return false;
}

int IRRecv::setPreferred(const char* timing_group)
{
    if(timing_group == NULL) {
        _preferred.clear();
        return 0;
    }
    int position = timingGroupElement(timing_group);
    if(position < 0) {
        return -1;
    } else { 
        if (!inPrefVector(position)) _preferred.push_back(position);
        return _preferred.size();
    } 
}
int IRRecv::setPreferred(String timing_group)
    {return setPreferred(timing_group.c_str());}

void IRRecv::stop()
{
    rmt_driver_uninstall(_channel);
    _rx_pin = GPIO_NUM_MAX;
    _timing = {};
    _active = false;
}

bool IRRecv::active() {return _active;}    
