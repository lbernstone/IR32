#include "IRSend.h"
#include "driver/rmt.h"

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

bool IRSend::start(const rmt_send_timing_t* timing_group, gpio_num_t tx_pin)
{
    if (tx_pin > 33) {
       log_e("Invalid pin for RMT TX: %d", tx_pin);
       return false;
    }
    rmt_config_t rmt_tx;
    rmt_tx.channel = _channel;
    rmt_tx.gpio_num = tx_pin;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.mem_block_num = 1;
    rmt_tx.rmt_mode = RMT_MODE_TX;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_freq_hz = timing_group->carrier_freq_khz * 1000;
    rmt_tx.tx_config.carrier_duty_percent = timing_group->duty_cycle;
    rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_tx.tx_config.idle_output_en = RMT_TX_IDLE_EN;
    if (rmt_config(&rmt_tx) != ESP_OK) return false;
    if (rmt_driver_install(rmt_tx.channel, 0, 0) != ESP_OK) return false;
    if (rmt_set_pin(_channel, RMT_MODE_TX, tx_pin) != ESP_OK) return false;
    _timing = *timing_group;
    _tx_pin = tx_pin;
    _active = true;
}

bool IRSend::start(const rmt_send_timing_t* timing_group, int tx_pin)
{
    return start(timing_group, (gpio_num_t) tx_pin);
}

void IRSend::rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}

void IRSend::rmt_fill_item_header(rmt_item32_t* item)
{
    rmt_fill_item_level(item, _timing.header_mark_us, _timing.header_space_us);
}

void IRSend::rmt_fill_item_bit_one(rmt_item32_t* item)
{
    rmt_fill_item_level(item, _timing.one_mark_us, _timing.one_space_us);
}

void IRSend::rmt_fill_item_bit_zero(rmt_item32_t* item)
{
    rmt_fill_item_level(item, _timing.zero_mark_us, _timing.zero_space_us);
}

void IRSend::rmt_fill_item_end(rmt_item32_t* item)
{
    rmt_fill_item_level(item, _timing.end_wait_us, RMT_TX_END_SPACE);
}

void IRSend::nec_build_item(rmt_item32_t* item, uint32_t cmd_data)
{}

void IRSend::rmt_build_item(rmt_item32_t* item, uint32_t cmd_data)
{
  rmt_fill_item_header(item++);
  
  // parse from left to right up to 32 bits (0x80000000)
  for (unsigned long mask = 1UL << (_timing.bit_length -1); mask; mask >>= 1) {
    if (cmd_data & mask) {
      if (_timing.invert) {
          rmt_fill_item_bit_zero(item);
      } else {
         rmt_fill_item_bit_one(item);
      }
    } else {
      if (_timing.invert) {
          rmt_fill_item_bit_one(item);
      } else {
         rmt_fill_item_bit_zero(item);
      }
    }
    item++;
  }

  rmt_fill_item_end(item);
}

bool IRSend::send(uint32_t code)
{
	size_t size = sizeof(rmt_item32_t) * (_timing.bit_length + 2);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	memset((void*) item, 0, size);
	
    if (_timing.tag == "xNEC") {nec_build_item(item,code);
      } else {rmt_build_item(item, code);}
    
    for (int x=0; x<_timing.bit_length+3; x++) {
        Serial.printf("item: %d  time0: %d  time1: %d\n", x, item[x].duration0, item[x].duration1);
    }
	int item_num = _timing.bit_length + 2;
	if (rmt_write_items(_channel, item, item_num, true) != ESP_OK) return false;
	rmt_wait_tx_done(_channel,RMT_TX_WAIT);
	free(item);
}

void IRSend::stop() 
{
    rmt_driver_uninstall(_channel);
    _timing = {};
    _tx_pin = GPIO_NUM_MAX;
    _active = false;
}

bool IRSend::active() {return _active;}
