# ESP32 RMT peripheral IR library

Arduino friendly IR library utilizing ESP32 RMT peripheral

Protocols supported (send, limited receive):
- NEC
- SAMSUNG
- RC5

### What's new
IR code is now decoded correctly (NEC and SAMSUNG, RC5 needs a bit of extra work).

NEC sometimes decodes odd values, but might be down to config or cheap remotes.

I am happy with the prototype. This is the last commit before a big refactoring.

## Example

```
#include <RMTLib.h>

RMTLib RMTLib;

const int TX_PIN = 27;

void setup()
{
  Serial.begin(115200);
  
  RMTLib.setTxPin(TX_PIN);
}

void loop()
{
  Serial.println("sending command");

  RMTLib.sendNEC(0x5743C03F);
  delay(5000);
}
```

## TODO
- Receive and decode (Arduino)
- Repeat/toggle functionality
- Refactoring current prototype level code

## Planned
- Examples to send/decode IR code through WIFI (Web, MQTT or CoAP)

## History

#### Version 0.3b
- Added SAMSUNG decode and started on RC5 decode

#### Version 0.3
- More reliable receive code (NEC only)
- Added extra debugging

#### Version 0.2a
- Added initial receive code for NEC (IR code is decoded but bits are reversed)
- Massive code refactoring (planning to support Arduino IDE + ESP-IDF toolchain with Eclipse)
- Fixed RMT_CLK_DIV, 80 instead of 100

#### Version 0.2
- GPIO can now be set at the beginning

#### Version 0.1
Initial version
- Arduino library with example sketch
- Send only

## Credits
Based on ideas from
- Arduino-IRremote library
- ESP-IDF infrared_nec_main.c example
- Various RMT examples on github
# IR32
