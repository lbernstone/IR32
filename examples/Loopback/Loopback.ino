#define IR_RMT_SEND_SAMSUNG 1
#include <IRSend.h>

IRSend remote1;

void setup() {
  remote1.start(&SAMSUNG_timing, 25);
}

void loop() {
  remote1.send(0x88);
  delay(2000);
}
