/*
8 bit address and 8 bit command length.
Address and command are transmitted twice for reliability. (32bit)
Extended mode available, doubling the address size.
Pulse distance modulation.
Carrier frequency of 38kHz.
Header is 9000 + 4500
Bit time of 1.125ms or 2.25ms
Each pulse is a 560us long 38kHz carrier burst (about 21 cycles).
A logical "1" takes 2.25ms to transmit, while a logical "0" is only half of that, being 1.125ms. 
The recommended carrier duty-cycle is 1/4 or 1/3.
*/
//#include "Arduino.h"
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

const char* NEC_TAG = "NEC";

/* NEC Protocol */
#define NEC_BITS              	32
#define NEC_HEADER_HIGH_US    	9000		/*!< header: positive 9ms */
#define NEC_HEADER_LOW_US     	4500        /*!< header: negative 4.5ms*/
#define NEC_BIT_ONE_HIGH_US    	560         /*!< data bit 1: positive 0.56ms */
#define NEC_BIT_ONE_LOW_US    	1690		/*!< data bit 1: negative 1.69ms */
#define NEC_BIT_ZERO_HIGH_US   	560         /*!< data bit 0: positive 0.56ms */
#define NEC_BIT_ZERO_LOW_US    	560			/*!< data bit 0: negative 0.56ms */
#define NEC_BIT_END            	560         /*!< end: positive 0.56ms */

#define NEC_BIT_MARGIN         	60         /*!< NEC parse error margin time */
#define NEC_DATA_ITEM_NUM   	34			/*!< NEC code item number: header + 32bit data + end */

/*
 * SEND
 */

#if SEND_NEC

void nec_fill_item_header(rmt_item32_t* item)
{
    rmt_fill_item_level(item, NEC_HEADER_HIGH_US, NEC_HEADER_LOW_US);
}

void nec_fill_item_bit_one(rmt_item32_t* item)
{
    rmt_fill_item_level(item, NEC_BIT_ONE_HIGH_US, NEC_BIT_ONE_LOW_US);
}

void nec_fill_item_bit_zero(rmt_item32_t* item)
{
    rmt_fill_item_level(item, NEC_BIT_ZERO_HIGH_US, NEC_BIT_ZERO_LOW_US);
}

void nec_fill_item_end(rmt_item32_t* item)
{
    rmt_fill_item_level(item, NEC_BIT_END, 0x7fff);
}

void nec_build_items(rmt_item32_t* item, uint32_t cmd_data)
{
  nec_fill_item_header(item++);
  
  // parse from left to right 32 bits (0x80000000)
  uint32_t mask = 0x01;
  mask <<= NEC_BITS - 1;
  //uint32_t mask = 0x80000000;

  for (int j = 0; j < 32; j++) {
    if (cmd_data & mask) {
      nec_fill_item_bit_one(item);
    } else {
      nec_fill_item_bit_zero(item);
    }
    item++;
    mask >>= 1;
  }

  nec_fill_item_end(item);
}

void nec_tx_init(gpio_num_t gpio_num)
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

void rmtlib_nec_send(unsigned long data)
{
	// what is the reason for this?
	vTaskDelay(10);
	nec_tx_init(RMT_TX_GPIO_NUM);

	esp_log_level_set(NEC_TAG, ESP_LOG_INFO);
	ESP_LOGI(NEC_TAG, "RMT TX DATA");

	size_t size = (sizeof(rmt_item32_t) * NEC_DATA_ITEM_NUM * RMT_TX_DATA_NUM);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	memset((void*) item, 0, size);

	nec_build_items(item, data);

	int item_num = NEC_DATA_ITEM_NUM * RMT_TX_DATA_NUM;
	rmt_write_items(RMT_TX_CHANNEL, item, item_num, true);
	rmt_wait_tx_done(RMT_TX_CHANNEL,RMT_TX_WAIT);
	free(item);
}

#endif

/*
 * RECEIVE
 */

#if RECEIVE_NEC

static bool nec_header_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, NEC_HEADER_HIGH_US, NEC_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, NEC_HEADER_LOW_US, NEC_BIT_MARGIN)) {

    	//ESP_LOGI(NEC_TAG, "Header: (%d) %dms, (%d) %dms", item->level0, item->duration0, item->level1, item->duration1);
        return true;
    }
    return false;
}

static bool nec_bit_one_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, NEC_BIT_ONE_HIGH_US, NEC_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, NEC_BIT_ONE_LOW_US, NEC_BIT_MARGIN)) {

    	//ESP_LOGI(NEC_TAG, "One: (%d) %dms, (%d) %dms", item->level0, item->duration0, item->level1, item->duration1);
        return true;
    }
    return false;
}

static bool nec_bit_zero_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, NEC_BIT_ZERO_HIGH_US, NEC_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, NEC_BIT_ZERO_LOW_US, NEC_BIT_MARGIN)) {

    	//ESP_LOGI(NEC_TAG, "Zero: (%d) %dms, (%d) %dms", item->level0, item->duration0, item->level1, item->duration1);
        return true;
    }
    return false;
}

static bool nec_end_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && rmt_check_in_range(item->duration0, NEC_BIT_END, NEC_BIT_MARGIN)
        && rmt_check_in_range(item->duration1, 0, NEC_BIT_MARGIN)) {

    	//ESP_LOGI(NEC_TAG, "Zero: (%d) %dms, (%d) %dms", item->level0, item->duration0, item->level1, item->duration1);
        return true;
    }
    return false;
}

static int nec_parse_items(rmt_item32_t* item, int item_num, uint32_t* cmd_data)
{
	//ESP_LOGI(NEC_TAG, "Parse %d items", item_num);

    if(item_num < NEC_DATA_ITEM_NUM) {
    	ESP_LOGI(NEC_TAG, "ITEM NUMBER ERROR");
        return -1;
    }

    int i = 0;

    if(!nec_header_if(item++)) {
    	ESP_LOGI(NEC_TAG, "HEADER ERROR");
        return -1;
    } else {
    	i++;
    }

    uint32_t mask = 0x80000000;
    uint32_t decoded = 0;

    for (int j = 0; j < NEC_BITS; j++) {

    	if(nec_bit_one_if(item)) {
    		decoded |= (mask >> j);
    	} else if (nec_bit_zero_if(item)) {
    		decoded |= 0 & (mask >> j);
        } else {
        	ESP_LOGI(NEC_TAG, "BIT ERROR");
            return -1;
        }
    	item++;
    	i++;
    }

    if (!nec_end_if(item++)) {
    	ESP_LOGI(NEC_TAG, "END MARKER ERROR");
    	return -1;
    } else {
    	i++;
    }

    *cmd_data = decoded;

    return i;
}

static void nec_rx_init()
{
	// idle_treshhold: In receive mode, when no edge is detected on the input signal for longer
	// than idle_thres channel clock cycles, the receive process is finished.

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

void rmtlib_nec_receive()
{
	vTaskDelay(10);
	ESP_LOGI(NEC_TAG, "RMT RX DATA");
	nec_rx_init();

    //get RMT RX ringbuffer
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handler(RMT_RX_CHANNEL, &rb);

	// rmt_rx_start(channel, rx_idx_rst) - Set true to reset memory index for receiver
    rmt_rx_start(RMT_RX_CHANNEL, 1);

    while(rb) {
        size_t rx_size = 0;
        //try to receive data from ringbuffer.
        //RMT driver will push all the data it receives to its ringbuffer.
        //We just need to parse the value and return the spaces of ringbuffer.
        rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 2000);
        if(item) {
        	ESP_LOGI(NEC_TAG, "Received waveform - buffer size: %d (%d items)", rx_size, rx_size / 4);

        	rmt_dump_items(item, rx_size / 4);

            uint32_t rmt_data;
            int res = nec_parse_items(item, rx_size / 4, &rmt_data);
            ESP_LOGI(NEC_TAG, "IR CODE: 0x%08x", rmt_data);
            /*
            int offset = 0;
            while(1) {
                //parse data value from ringbuffer.
                int res = nec_parse_items(item + offset, rx_size / 4 - offset, &rmt_data);
                ESP_LOGI(NEC_TAG, "RMT RCV --- res after parse: %d", res);

                if(res == NEC_DATA_ITEM_NUM) {
                    offset += res + 1;
                    ESP_LOGI(NEC_TAG, "RMT RCV --- IRCODE: 0x%08x", rmt_data);
                } else {
                	ESP_LOGI(NEC_TAG, "RMT RCV --- %d <= 0", res);
                    break;
                }
            }
			*/

            //after parsing the data, free ringbuffer.
            vRingbufferReturnItem(rb, (void*) item);
        } else {
        	ESP_LOGI(NEC_TAG, "ELSE (ITEM)");
            break;
        }
    }

    ESP_LOGI(NEC_TAG, "TASK DELETE");
    //vTaskDelete(NULL);
    rmt_driver_uninstall(RMT_RX_CHANNEL);
}

#endif
