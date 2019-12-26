#include "IRSend.h"

#define RMT_TX_WAIT 250
#define RMT_TX_DATA_NUM 1
#define RMT_TX_CARRIER_EN 1
#define RMT_TX_IDLE_EN    1
#define RMT_TX_END_SPACE 0x7FFF

IRSend::IRSend(rmt_channel_t channel) 
{
    if (channel >= RMT_CHANNEL_MAX) {
        log_e("Invalid RMT channel: %d", channel);
    } else {
        _channel = channel;
    }
    _timing = {};
    _tx_pin = GPIO_NUM_MAX;
}

uint8_t findGroup(const char* timingGroup)
{
    uint8_t counter = 0;
    for (rmt_timing_t timing : timing_groups) {
        if (timing.tag == timingGroup) return counter;
        counter++;
    }
    return 0;
}

bool IRSend::startRMT(uint8_t timing)
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = _channel;
    rmt_tx.gpio_num = _tx_pin;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.mem_block_num = 1;
    rmt_tx.rmt_mode = RMT_MODE_TX;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_freq_hz = timing_groups[timing].carrier_freq_hz;
    rmt_tx.tx_config.carrier_duty_percent = timing_groups[timing].duty_cycle;
    rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_tx.tx_config.idle_output_en = RMT_TX_IDLE_EN;
    if (rmt_config(&rmt_tx) != ESP_OK) return false;
    if (rmt_driver_install(rmt_tx.channel, 0, 0) != ESP_OK) return false;
    if (rmt_set_pin(_channel, RMT_MODE_TX, _tx_pin) != ESP_OK) return false;
    _timing = timing;
    return true;
}

bool IRSend::start(gpio_num_t tx_pin, const char* timingGroup)
{
    if (tx_pin > 33) {
       log_e("Invalid pin for RMT TX: %d", tx_pin);
       return false;
    }
    _timing = findGroup(timingGroup);
    if (!_timing) {
        log_e("Invalid timing group requested: %s", timingGroup);
        return false;
    }
    _tx_pin = tx_pin;
    if (!startRMT(_timing)) return false;
    _active = true;
    return true;
}
bool IRSend::start(gpio_num_t tx_pin, String timingGroup)
    {return start(tx_pin, timingGroup.c_str());}
bool IRSend::start(int tx_pin, const char* timingGroup)
    {return start((gpio_num_t)tx_pin, timingGroup);}
bool IRSend::start(int tx_pin, String timingGroup)
    {return start((gpio_num_t)tx_pin, timingGroup.c_str());}

void IRSend::rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}

void IRSend::rmt_fill_item_header(rmt_item32_t* item)
{
    rmt_fill_item_level(item, timing_groups[_timing].header_mark_us, timing_groups[_timing].header_space_us);
}

void IRSend::rmt_fill_item_bit_one(rmt_item32_t* item)
{
    rmt_fill_item_level(item, timing_groups[_timing].one_mark_us, timing_groups[_timing].one_space_us);
}

void IRSend::rmt_fill_item_bit_zero(rmt_item32_t* item)
{
    rmt_fill_item_level(item, timing_groups[_timing].zero_mark_us, timing_groups[_timing].zero_space_us);
}

void IRSend::rmt_fill_item_end(rmt_item32_t* item)
{
    rmt_fill_item_level(item, timing_groups[_timing].end_wait_us, RMT_TX_END_SPACE);
}

void IRSend::rmt_build_item(rmt_item32_t* item, uint32_t cmd_data)
{
  rmt_fill_item_header(item++);
  
  // parse from left to right up to 32 bits (0x80000000)
  for (unsigned long mask = 1UL << (timing_groups[_timing].bit_length -1); mask; mask >>= 1) {
    if (cmd_data & mask) {
      if (timing_groups[_timing].invert) {
          rmt_fill_item_bit_zero(item);
      } else {
         rmt_fill_item_bit_one(item);
      }
    } else {
      if (timing_groups[_timing].invert) {
          rmt_fill_item_bit_one(item);
      } else {
         rmt_fill_item_bit_zero(item);
      }
    }
    item++;
  }

  rmt_fill_item_end(item);
}

bool IRSend::send(uint32_t code, uint8_t timing)
{
    if (!active()) {
        log_e("IRSend has not been started");
        return false;
    }
    if (!timing) {
        log_e("Invalid timing group requested: %d", timing);
        return false;
    }
    if (timing != _timing) {startRMT(timing);}
	size_t size = sizeof(rmt_item32_t) * (timing_groups[timing].bit_length + 2);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	memset((void*) item, 0, size);
	
    rmt_build_item(item, code);
    
    for (int x=0; x<timing_groups[timing].bit_length+3; x++) {
        //Serial.printf("item: %d  time0: %d  time1: %d\n", x, item[x].duration0, item[x].duration1);
    }
	int item_num = timing_groups[timing].bit_length + 2;
	if (rmt_write_items(_channel, item, item_num, true) != ESP_OK) return false;
	rmt_wait_tx_done(_channel,RMT_TX_WAIT);
	free(item);
    return true;
}
bool IRSend::send(uint32_t code, const char* timingGroup)
    {return send(code, findGroup(timingGroup));}
bool IRSend::send(uint32_t code)
    {return send(code, _timing);}
bool IRSend::send(std::string code, uint8_t timing)
{
    bool success = true;
    if (code.empty())
    {
        log_e("String is empty. No message sent.");
        return false;
    }

    for (int i=0; i<code.length(); i=i+4)
    {
        uint32_t code_part = 0;
        try
        {
            code_part = code.at(i) << 24;
            code_part = code.at(i+1) << 16;
            code_part = code.at(i+2) << 8;
            code_part = code.at(i+3);
        }
        catch (const std::out_of_range& oor)
        {
            log_w("code incomplete");
        }
        log_i("sending code 0x%x", code_part); //TODO: print format not correct
        success = success & send(code_part, timing);
    }
    return success;
}
bool IRSend::send(std::string code)
    {return send(code, _timing);}

void IRSend::stop() 
{
    rmt_driver_uninstall(_channel);
    _timing = {};
    _tx_pin = GPIO_NUM_MAX;
    _active = false;
}

bool IRSend::active() {return _active;}
