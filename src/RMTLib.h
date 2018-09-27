/*
 * Protocols:
 *  - Pulse Distance Modulation (NEC)
 *  - Manchester Encoding (RC5)
 *
 */
 
#ifndef	RMTLib_H
#define	RMTLib_H

#include "rmtlib/esp32_rmt_remotes.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Include C code here */
#ifdef __cplusplus
}
#endif

class RMTLib {
	public:
		RMTLib();
		
		gpio_num_t tx_pin;
		gpio_num_t rx_pin;
		
		void send();
		void setTxPin(unsigned short pin);
		void setRxPin(unsigned short pin);
		
#if SEND_NEC
		//void initNEC(unsigned short pin);
		void sendNEC(unsigned long data);
#endif

#if RECEIVE_NEC
		void decodeNEC();
#endif

#if SEND_SAMSUNG		
		//void initSAMSUNG(unsigned short pin);
		void sendSAMSUNG(unsigned long data);
#endif

#if RECEIVE_SAMSUNG
		void decodeSAMSUNG();
#endif

#if SEND_RC5
		//void initRC5(unsigned short pin);
		void sendRC5(unsigned long data);
#endif

#if RECEIVE_RC5
		void decodeRC5();
#endif
		
	private:
		//

};

#endif
