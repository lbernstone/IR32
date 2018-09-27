/*
 *
 *
 */

#include "Arduino.h"
#include "RMTLib.h"

#include "rmtlib/esp32_rmt_remotes.h"
#include "rmtlib/esp32_rmt_common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Include C code here */
#ifdef __cplusplus
}
#endif


RMTLib::RMTLib () { 
	tx_pin = RMT_TX_GPIO_NUM;
	rx_pin = RMT_RX_GPIO_NUM;
}
	
void RMTLib::send(void) {
	
}

void RMTLib::setTxPin(unsigned short pin) {
	tx_pin = (gpio_num_t)pin;
	//rmt_set_tx_pin((gpio_num_t)pin);
}

void RMTLib::setRxPin(unsigned short pin) {
	rx_pin = (gpio_num_t)pin;
}

#if SEND_NEC
void RMTLib::sendNEC(unsigned long data)
{
	Serial.println("sendNEC");
	rmtlib_nec_send(data);
}
#endif

#if RECEIVE_NEC
void RMTLib::decodeNEC()
{
	Serial.println("**decodeNEC");
	rmtlib_nec_receive();
}
#endif

#if SEND_SAMSUNG
void RMTLib::sendSAMSUNG(unsigned long data)
{
	Serial.println("sendSAMSUNG");
	rmtlib_samsung_send(data);
}
#endif

#ifdef RECEIVE_SAMSUNG
void RMTLib::decodeSAMSUNG()
{
	Serial.println("**decodeSAMSUNG");
	rmtlib_samsung_receive();
}
#endif

#if SEND_RC5
void RMTLib::sendRC5 (unsigned long data)
{
	Serial.println("sendRC5");
	rmtlib_rc5_send(data);
}
#endif

#ifdef RECEIVE_RC5
void RMTLib::decodeRC5()
{
	Serial.println("**decodeRC5");
	rmtlib_rc5_receive();
}
#endif

// Private


