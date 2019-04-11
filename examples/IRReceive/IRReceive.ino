#define RX_PIN 34

#include <IRRecv.h>

IRRecv remote2;

void setup() {
  Serial.begin(115200);
  remote2.start(RX_PIN);
}

void loop() { 
  while(remote2.available()){
    char* rcvGroup;
    uint32_t result = remote2.read(rcvGroup);
    if (result) {
        Serial.printf("Received: %s/0x%x\n", rcvGroup, result);
    }
  }  
  delay(100);
}
