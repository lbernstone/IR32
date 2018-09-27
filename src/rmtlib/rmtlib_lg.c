/*
Like NEC but a wee bit different header timings (5000 + 5000)
*/
#include "esp32_rmt_common.h"
#include "esp32_rmt_remotes.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include "freertos/ringbuf.h"


#define LG_BITS              	28
#define LG_HEADER_HIGH_US    	8500
#define LG_HEADER_LOW_US     	4250
#define LG_BIT_ONE_HIGH_US    	560
#define LG_BIT_ONE_LOW_US    	1600
#define LG_BIT_ZERO_HIGH_US   	560
#define LG_BIT_ZERO_LOW_US    	560
#define LG_BIT_END            	800
#define LG_32_BITS              	32
#define LG_32_HEADER_HIGH_US    	4500
#define LG_32_HEADER_LOW_US     	4450
#define LG_32_BIT_ONE_HIGH_US    	560
#define LG_32_BIT_ONE_LOW_US    	1600
#define LG_32_BIT_ZERO_HIGH_US   	560
#define LG_32_BIT_ZERO_LOW_US    	560
#define LG_32_BIT_END            	8950
//
#define LG_DATA_ITEM_NUM   	30  /*!< code item number: header + 28bit data + end */
#define LG_32_DATA_ITEM_NUM   	34  /*!< code item number: header + 32bit data + end */
#define LG_BIT_MARGIN			60  /* deviation from signal timing */

const char* LG_TAG = "LG";

/*
 * SEND
 */

#if SEND_LG

void lg_fill_item_header(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_HEADER_HIGH_US, LG_HEADER_LOW_US);
}

void lg_fill_item_bit_one(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_BIT_ONE_HIGH_US, LG_BIT_ONE_LOW_US);
}

void lg_fill_item_bit_zero(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_BIT_ZERO_HIGH_US, LG_BIT_ZERO_LOW_US);
}

void lg_fill_item_end(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_BIT_END, 0x7fff);
}

void lg_build_items(rmt_item32_t* item, uint32_t cmd_data)
{
  lg_fill_item_header(item++);
  
  // parse from left to right 32 bits (0x80000000)
  uint32_t mask = 0x01;
  mask <<= LG_BITS - 1;

  for (int j = 0; j < 32; j++) {
    if (cmd_data & mask) {
      lg_fill_item_bit_one(item);
    } else {
      lg_fill_item_bit_zero(item);
    }
    item++;
    mask >>= 1;
  }

  lg_fill_item_end(item);
}

void lg_tx_init(gpio_num_t gpio_num)
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = RMT_TX_CHANNEL;
    rmt_tx.gpio_num = gpio_num;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = RMT_CARRIER_DUTY;
    rmt_tx.tx_config.carrier_freq_hz = RMT_CARRIER_FREQ_38;
    rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_tx.tx_config.idle_output_en = true;
    rmt_tx.rmt_mode = RMT_MODE_TX;
	
    rmt_tx_init(&rmt_tx);
}

void rmtlib_lg_send(unsigned long data)
{
	vTaskDelay(10);
	lg_tx_init(RMT_TX_GPIO_NUM);
	
	esp_log_level_set(LG_TAG, ESP_LOG_INFO);
	ESP_LOGI(LG_TAG, "RMT TX DATA");
	
	size_t size = (sizeof(rmt_item32_t) * LG_DATA_ITEM_NUM * RMT_TX_DATA_NUM);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	memset((void*) item, 0, size);
	
	lg_build_items(item, data);

	int item_num = LG_DATA_ITEM_NUM * RMT_TX_DATA_NUM;
	rmt_write_items(RMT_TX_CHANNEL, item, item_num, true);
	rmt_wait_tx_done(RMT_TX_CHANNEL,RMT_TX_WAIT);
	free(item);
}

#endif

#if SEND_LG_32

void lg_32_fill_item_header(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_32_HEADER_HIGH_US, LG_32_HEADER_LOW_US);
}

void lg_32_fill_item_bit_one(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_32_BIT_ONE_HIGH_US, LG_32_BIT_ONE_LOW_US);
}

void lg_32_fill_item_bit_zero(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_32_BIT_ZERO_HIGH_US, LG_32_BIT_ZERO_LOW_US);
}

void lg_32_fill_item_end(rmt_item32_t* item)
{
    rmt_fill_item_level(item, LG_32_BIT_END, 0x7fff);
}

void lg_32_build_items(rmt_item32_t* item, uint32_t cmd_data)
{
  lg_32_fill_item_header(item++);
  
  // parse from left to right 32 bits (0x80000000)
  uint32_t mask = 0x01;
  mask <<= LG_32_BITS - 1;

  for (int j = 0; j < 32; j++) {
    if (cmd_data & mask) {
      lg_32_fill_item_bit_one(item);
    } else {
      lg_32_fill_item_bit_zero(item);
    }
    item++;
    mask >>= 1;
  }

  lg_32_fill_item_end(item);
}

void lg_32_tx_init(gpio_num_t gpio_num)
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = RMT_TX_CHANNEL;
    rmt_tx.gpio_num = gpio_num;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = RMT_CARRIER_DUTY;
    rmt_tx.tx_config.carrier_freq_hz = RMT_CARRIER_FREQ_38;
    rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_tx.tx_config.idle_output_en = true;
    rmt_tx.rmt_mode = RMT_MODE_TX;
	
    rmt_tx_init(&rmt_tx);
}

void rmtlib_lg_32_send(unsigned long data)
{
	vTaskDelay(10);
	lg_32_tx_init(RMT_TX_GPIO_NUM);
	
	esp_log_level_set(LG_32_TAG, ESP_LOG_INFO);
	ESP_LOGI(LG_32_TAG, "RMT TX DATA");
	
	size_t size = (sizeof(rmt_item32_t) * LG_32_DATA_ITEM_NUM * RMT_TX_DATA_NUM);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	memset((void*) item, 0, size);
	
	lg_32_build_items(item, data);

	int item_num = LG_32_DATA_ITEM_NUM * RMT_TX_DATA_NUM;
	rmt_write_items(RMT_TX_CHANNEL, item, item_num, true);
	rmt_wait_tx_done(RMT_TX_CHANNEL,RMT_TX_WAIT);
	free(item);
}

#endif

/*
 * RECEIVE
 */

#ifdef RECEIVE_LG

static bool samsung_header_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, LG_HEADER_HIGH_US, LG_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, LG_HEADER_LOW_US, LG_BIT_MARGIN)) {
        return true;
    }
    return false;
}

static bool samsung_bit_one_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, LG_BIT_ONE_HIGH_US, LG_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, LG_BIT_ONE_LOW_US, LG_BIT_MARGIN)) {
        return true;
    }
    return false;
}

static bool samsung_bit_zero_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, LG_BIT_ZERO_HIGH_US, LG_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, LG_BIT_ZERO_LOW_US, LG_BIT_MARGIN)) {
        return true;
    }
    return false;
}

static bool samsung_end_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, LG_BIT_END, LG_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, 0, LG_BIT_MARGIN)) {
        return true;
    }
    return false;
}

static int samsung_parse_items(rmt_item32_t* item, int item_num, uint32_t* cmd_data)
{
    if(item_num < LG_DATA_ITEM_NUM) {
    	ESP_LOGI(LG_TAG, "ITEM NUMBER ERROR");
        return -1;
    }

    int i = 0;

    if(!samsung_header_if(item++)) {
    	ESP_LOGI(LG_TAG, "HEADER ERROR");
        return -1;
    } else {
    	i++;
    }

    uint32_t decoded = 0;

    // parse from left to right 32 bits (0x80000000)
    uint32_t mask = 0x01;
    mask <<= LG_BITS - 1;

    for (int j = 0; j < LG_BITS; j++) {

    	if(samsung_bit_one_if(item)) {
    		decoded |= (mask >> j);
    	} else if (samsung_bit_zero_if(item)) {
    		decoded |= 0 & (mask >> j);
        } else {
        	ESP_LOGI(LG_TAG, "BIT ERROR");
            return -1;
        }
    	item++;
    	i++;
    }

    if (!samsung_end_if(item++)) {
    	ESP_LOGI(LG_TAG, "END MARKER ERROR");
    	return -1;
    } else {
    	i++;
    }

    *cmd_data = decoded;

    return i;
}

static void lg_rx_init()
{
    rmt_config_t rmt_rx;
    rmt_rx.channel = RMT_RX_CHANNEL;
    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = true;
    rmt_rx.rx_config.filter_ticks_thresh = 100;
    rmt_rx.rx_config.idle_threshold = rmt_item32_TIMEOUT_US / 10 * (RMT_TICK_10_US);

    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 2000, 0);
}

void rmtlib_lg_receive()
{
	vTaskDelay(10);
	ESP_LOGI(LG_TAG, "RMT RX DATA");
	lg_rx_init();

    //get RMT RX ringbuffer
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handler(RMT_RX_CHANNEL, &rb);

	// rmt_rx_start(channel, rx_idx_rst) - Set true to reset memory index for receiver
    rmt_rx_start(RMT_RX_CHANNEL, 1);

    while(rb) {
        size_t rx_size = 0;
        rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 2000);

        if(item) {
        	ESP_LOGI(LG_TAG, "Received waveform - buffer size: %d (%d items)", rx_size, rx_size / 4);

        	rmt_dump_items(item, rx_size / 4);

            uint32_t rmt_data;
#ifdef RECEIVE_LG
            int res = samsung_parse_items(item, rx_size / 4, &rmt_data);
#endif
#ifdef RECEIVE_LG_32
            if (!res) res = lg_32_parse_items(item, rx_size/4, &rmt_data);
#endif
            ESP_LOGI(LG_TAG, "IR CODE: 0x%08x", rmt_data);
            vRingbufferReturnItem(rb, (void*) item);
        } else {
        	ESP_LOGI(LG_TAG, "ELSE (ITEM)");
            break;
        }
    }

    ESP_LOGI(LG_TAG, "END");
    rmt_driver_uninstall(RMT_RX_CHANNEL);
}

#endif
