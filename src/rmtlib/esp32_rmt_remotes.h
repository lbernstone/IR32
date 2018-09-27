/*
 * esp32_rmt_remotes.h
 *
 */

#ifndef ESP32_RMT_REMOTES_H
#define ESP32_RMT_REMOTES_H

/* Available remote protocols */
#define SEND_NEC			1
#define RECEIVE_NEC			1
#define SEND_SAMSUNG		1
#define RECEIVE_SAMSUNG		1
#define SEND_RC5			1
#define RECEIVE_RC5			1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SEND_NEC
void rmtlib_nec_send(unsigned long data);
#endif

#ifdef RECEIVE_NEC
void rmtlib_nec_receive();
#endif

#ifdef SEND_SAMSUNG
void rmtlib_samsung_send(unsigned long data);
#endif

#ifdef RECEIVE_SAMSUNG
void rmtlib_samsung_receive();
#endif

#ifdef SEND_RC5
void rmtlib_rc5_send(unsigned long data);
#endif

#ifdef RECEIVE_RC5
void rmtlib_rc5_receive();
#endif

#ifdef __cplusplus
} // extern C
#endif


#endif /* ESP32_RMT_REMOTES_H */
